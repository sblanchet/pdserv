/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdserv library.
 *
 *  The pdserv library is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or (at
 *  your option) any later version.
 *
 *  The pdserv library is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with the pdserv library. If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include "config.h"

#include "Server.h"
#include "Channel.h"
#include "Event.h"
#include "TimeSignal.h"
#include "StatSignal.h"
#include "Parameter.h"
#include "Session.h"
#include "../Main.h"
#include "../Task.h"
#include "../Signal.h"
#include "../ProcessParameter.h"

#include <cerrno>
#include <algorithm>
#include <cc++/socketport.h>
#include <log4cplus/loggingmacros.h>

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Server::Server(const PdServ::Main *main, const PdServ::Config &config):
    main(main),
    log(log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("msr"))),
    active(&_active),
    mutex(1)
{
    port = config["port"].toUInt();
    itemize = config["splitvectors"].toUInt();

    log_debug("port=%u", port);

    insertRoot = &variableDirectory;
    if (config["pathprefix"])
        insertRoot = variableDirectory.create(
                config["pathprefix"].toString(main->name));

    for (size_t i = 0; i < main->numTasks(); ++i)
        createChannels(insertRoot, i);

    createParameters(insertRoot);
    createEvents();

    start();
}

/////////////////////////////////////////////////////////////////////////////
Server::~Server()
{
    terminate();

    // Parameters and Channels are deleted when variable tree
    // is deleted
}

/////////////////////////////////////////////////////////////////////////////
void Server::createChannels(DirectoryNode* baseDir, size_t taskIdx)
{
    const PdServ::Task *task = main->getTask(taskIdx);
    Channel* c;

    LOG4CPLUS_TRACE(log,
            LOG4CPLUS_TEXT("Create channels for task ") << taskIdx
            << LOG4CPLUS_TEXT(", Ts=") << task->sampleTime);

    const PdServ::Task::Signals& signals = task->getSignals();

    // Reserve at least signal count and additionally 4 Taskinfo signals
    channels.reserve(channels.size() + signals.size() + 4);

    for (PdServ::Task::Signals::const_iterator it = signals.begin();
            it != signals.end(); ++it) {
        const PdServ::Signal *signal = *it;

        LOG4CPLUS_TRACE(log, LOG4CPLUS_TEXT(signal->path)
                << signal->dtype.name);
        PdServ::DataType::Iterator<CreateChannel>(
                signal->dtype, signal->dim,
                CreateChannel(this, baseDir, taskIdx, signal));
    }

    DirectoryNode* taskInfo = variableDirectory.create("Taskinfo");

    std::ostringstream os;
    os << taskIdx;
    DirectoryNode* t = taskInfo->create(os.str());

    c = new TimeSignal(taskIdx, task, channels.size());
    channels.push_back(c);
    t->insert(c, "TaskTime");

    c = new StatSignal(taskIdx, task, StatSignal::ExecTime, channels.size());
    channels.push_back(c);
    t->insert(c, "ExecTime");

    c = new StatSignal(taskIdx, task, StatSignal::Period, channels.size());
    channels.push_back(c);
    t->insert(c, "Period");

    c = new StatSignal(taskIdx, task, StatSignal::Overrun, channels.size());
    channels.push_back(c);
    t->insert(c, "Overrun");
}

/////////////////////////////////////////////////////////////////////////////
void Server::createEvents ()
{
    LOG4CPLUS_TRACE(log,
            LOG4CPLUS_TEXT("Create events"));

    const PdServ::Main::Events mainEvents = main->getEvents();
    this->events.reserve(mainEvents.size());

    for (PdServ::Main::Events::const_iterator it = mainEvents.begin();
            it != mainEvents.end(); ++it)
        this->events.push_back(new Event(*it));
}

/////////////////////////////////////////////////////////////////////////////
void Server::createParameters(DirectoryNode* baseDir)
{
    LOG4CPLUS_TRACE(log,
            LOG4CPLUS_TEXT("Create parameters"));

    const PdServ::Main::ProcessParameters& mainParam = main->getParameters();

    parameters.reserve(parameters.size() + mainParam.size());

    PdServ::Main::ProcessParameters::const_iterator it;
    for (it = mainParam.begin(); it != mainParam.end(); ++it) {
        const PdServ::Parameter *param = *it;
        PdServ::DataType::Iterator<CreateParameter>(
                param->dtype, param->dim,
                CreateParameter(this, baseDir, param));
    }
}

/////////////////////////////////////////////////////////////////////////////
void Server::initial()
{
    LOG4CPLUS_INFO_STR(log, LOG4CPLUS_TEXT("Initializing MSR server"));
    _active = true;
}

/////////////////////////////////////////////////////////////////////////////
void Server::final()
{
    _active = false;

    while (!sessions.empty())
        Thread::sleep(100);

    LOG4CPLUS_INFO_STR(log, LOG4CPLUS_TEXT("Exiting MSR server"));
}

/////////////////////////////////////////////////////////////////////////////
void Server::run()
{
    ost::TCPSocket *server = 0;

    ost::tpport_t port = this->port ? this->port : 2345;

    do {
        try {
            server = new ost::TCPSocket(ost::IPV4Address("0.0.0.0"), port);
            LOG4CPLUS_INFO(log,
                    LOG4CPLUS_TEXT("Server started on port ") << port);
        }
        catch (ost::Socket *s) {
            long err = s->getSystemError();
            if (this->port or err != EADDRINUSE) {
                LOG4CPLUS_ERROR(log,
                        LOG4CPLUS_TEXT("Socket failure on port ") << port
                        << LOG4CPLUS_TEXT(": ")
                        << LOG4CPLUS_C_STR_TO_TSTRING(::strerror(err)));
                return;
            }
            LOG4CPLUS_DEBUG(log,
                    LOG4CPLUS_TEXT("Port ") << port
                    << LOG4CPLUS_TEXT(" is busy. Trying next one..."));
            port++;
        }
    } while (!server);

    while (server->isPendingConnection()) {
        try {
            LOG4CPLUS_TRACE_STR(log,
                    LOG4CPLUS_TEXT("New client connection"));
            ost::SemaphoreLock lock(mutex);
            sessions.insert(new Session(this, server));
        }
        catch (ost::Socket *s) {
            LOG4CPLUS_FATAL(log,
                    LOG4CPLUS_TEXT("Socket failure: ")
                    << LOG4CPLUS_C_STR_TO_TSTRING(
                        ::strerror(s->getSystemError())));
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
void Server::broadcast(Session *s, const struct timespec& ts,
        const std::string& action, const std::string &message)
{
    ost::SemaphoreLock lock(mutex);
    for (std::set<Session*>::iterator it = sessions.begin();
            it != sessions.end(); ++it)
        (*it)->broadcast(s, ts, action, message);
}

/////////////////////////////////////////////////////////////////////////////
void Server::sessionClosed(Session *s)
{
    ost::SemaphoreLock lock(mutex);
    sessions.erase(s);
}

/////////////////////////////////////////////////////////////////////////////
void Server::getSessionStatistics(
        std::list<PdServ::SessionStatistics>& stats) const
{
    ost::SemaphoreLock lock(mutex);
    for (std::set<Session*>::iterator it = sessions.begin();
            it != sessions.end(); ++it) {
        PdServ::SessionStatistics s;
        (*it)->getSessionStatistics(s);
        stats.push_back(s);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Server::setAic(const Parameter *p)
{
    ost::SemaphoreLock lock(mutex);
    for (std::set<Session*>::iterator it = sessions.begin();
            it != sessions.end(); ++it)
        (*it)->setAIC(p);
}

/////////////////////////////////////////////////////////////////////////////
void Server::parameterChanged(const PdServ::Parameter *mainParam,
        size_t offset, size_t count)
{
    const Parameter *p = find(mainParam);

    ost::SemaphoreLock lock(mutex);
    for (std::set<Session*>::iterator it = sessions.begin();
            it != sessions.end(); ++it)
        p->inform(*it, offset, offset + count);
}

/////////////////////////////////////////////////////////////////////////////
const Channel* Server::getChannel(size_t n) const
{
    return n < channels.size() ? channels[n] : 0;
}

/////////////////////////////////////////////////////////////////////////////
const Server::Channels& Server::getChannels() const
{
    return channels;
}

/////////////////////////////////////////////////////////////////////////////
void Server::listDir(PdServ::Session *session,
        XmlElement& xml, const std::string& path) const
{
    variableDirectory.list(session, xml, path);
}

/////////////////////////////////////////////////////////////////////////////
const Parameter* Server::getParameter(size_t n) const
{
    return n < parameters.size() ? parameters[n] : 0;
}

/////////////////////////////////////////////////////////////////////////////
const Server::Parameters& Server::getParameters() const
{
    return parameters;
}

/////////////////////////////////////////////////////////////////////////////
const Parameter *Server::find( const PdServ::Parameter *p) const
{
    return parameterMap.find(p)->second;
}

/////////////////////////////////////////////////////////////////////////////
const Server::Events& Server::getEvents() const
{
    return events;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Server::CreateVariable::CreateVariable(
        Server* server, DirectoryNode* baseDir, const PdServ::Variable* var):
    parent(0), server(server), baseDir(baseDir), var(var)
{
}

/////////////////////////////////////////////////////////////////////////////
Server::CreateVariable::CreateVariable(const CreateVariable& other):
    parent(&other), server(other.server),
    baseDir(other.baseDir), var(other.var)
{
}

/////////////////////////////////////////////////////////////////////////////
std::string Server::CreateVariable::path() const
{
    return (parent ? parent->path() : var->path)
        + std::string(!name.empty(), '/') + name;
}

/////////////////////////////////////////////////////////////////////////////
void Server::CreateVariable::newDimension(
        const PdServ::DataType& /*dtype*/,
        const PdServ::DataType::DimType& /*dim*/,
        size_t /*dimIdx*/, size_t elemIdx,
        CreateVariable& /*c*/, size_t /*offset*/)
{
    std::ostringstream os;
    os << elemIdx;
    name = os.str();
}

/////////////////////////////////////////////////////////////////////////////
void Server::CreateVariable::newField(const PdServ::DataType::Field* field,
        CreateVariable& /*c*/, size_t /*offset*/)
{
    name = field->name;
}

/////////////////////////////////////////////////////////////////////////////
bool Server::CreateVariable::newVariable(
        const PdServ::DataType& dtype,
        const PdServ::DataType::DimType& dim,
        size_t dimIdx, size_t elemIdx, size_t offset) const
{
    bool rv = false;

    if (dtype.isPrimary()) {
        if ((dimIdx == 0 and elemIdx == 0) or !server->itemize) {
            // Root element of a primary variable
            rv = createVariable(dtype, dim, offset);
        }
        else if (dimIdx == dim.size()) {
            // Leaf node with only one element
            static const size_t d = 1;
            rv = createVariable(dtype,
                    PdServ::DataType::DimType(1, &d), offset);
        }
    }

    return rv;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Server::CreateChannel::CreateChannel(
        Server* server, DirectoryNode* baseDir,
        size_t taskIdx, const PdServ::Signal* s):
    CreateVariable(server, baseDir, s),
    taskIdx(taskIdx)
{
}

/////////////////////////////////////////////////////////////////////////////
bool Server::CreateChannel::createVariable(const PdServ::DataType& dtype,
        const PdServ::DataType::DimType& dim, size_t offset) const
{
    Channel *c = new Channel(taskIdx, static_cast<const PdServ::Signal*>(var),
            server->channels.size(), dtype, dim, offset);
    bool rv;

    if (server->itemize) {
        char hidden = 0;
        baseDir->traditionalPathInsert(c, path(), hidden);
        c->hidden = hidden == 'c' or hidden == 'k' or hidden == 1;

        rv = c->hidden;
    }
    else {
        baseDir->pathInsert(c, path());
        rv = true;
    }

    server->channels.push_back(c);
    return rv;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Server::CreateParameter::CreateParameter(
        Server* server, DirectoryNode* baseDir, const PdServ::Parameter* p):
    CreateVariable(server, baseDir, p), parentParameter(0)
{
}

/////////////////////////////////////////////////////////////////////////////
bool Server::CreateParameter::createVariable(const PdServ::DataType& dtype,
        const PdServ::DataType::DimType& dim, size_t offset) const
{
    Parameter *p = new Parameter(static_cast<const PdServ::Parameter*>(var),
            server->parameters.size(), dtype, dim, offset, parentParameter);
    bool rv;

    server->parameters.push_back(p);

    if (server->itemize) {
        char hidden = 0;
        baseDir->traditionalPathInsert(p, path(), hidden);
        p->hidden = hidden == 'p' or hidden == 1;

        rv = p->hidden;
    }
    else {
        baseDir->pathInsert(p, path());
        rv = true;
    }

    if (!parentParameter)
        server->parameterMap[static_cast<const PdServ::Parameter*>(var)] = p;
    parentParameter = p;

    return rv;
}
