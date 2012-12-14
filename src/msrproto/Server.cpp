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
#include "../Debug.h"

#include <cc++/socketport.h>
#include <algorithm>
#include <log4cpp/Category.hh>
#include <log4cpp/NDC.hh>

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Server::Server(const PdServ::Main *main,
        const PdServ::Config& defaultConfig, const PdServ::Config &config):
    main(main),
    log(log4cpp::Category::getInstance(main->name)),
    mutex(1)
{
    const PdServ::Config msrConfig = config["msr"];

    port = msrConfig["port"].toUInt();
    traditional = msrConfig["traditional"]
        ? msrConfig["traditional"].toUInt()
        : defaultConfig["traditional"].toUInt();

    log_debug("port=%u", port);

    DirectoryNode* baseDir = this;
    if (traditional and msrConfig["modelnameprefix"].toUInt())
        baseDir = create(config["name"].toString(main->name));

    for (size_t i = 0; i < main->numTasks(); ++i)
        createChannels(baseDir, i);

    createParameters(baseDir);
    createEvents(config["events"]);

    start();
}

/////////////////////////////////////////////////////////////////////////////
void Server::createChannels(DirectoryNode* baseDir, size_t taskIdx)
{
    const PdServ::Task *task = main->getTask(taskIdx);
    Channel* c;

    const PdServ::Task::Signals& signals = task->getSignals();

    // Reserve at least signal count and additionally 4 Taskinfo signals
    channels.reserve(channels.size() + signals.size() + 4);

    for (PdServ::Task::Signals::const_iterator it = signals.begin();
            it != signals.end(); it++) {
        c = new Channel(*it,
                channels.size(), (*it)->dtype, (*it)->dim, 0, 0);

        char hidden;
        if (traditional)
            baseDir->traditionalPathInsert(c, (*it)->path, hidden);
        else
            baseDir->pathInsert(c, (*it)->path);

        c->hidden = hidden == 'c' or hidden == 'k' or hidden == 1;

        channels.push_back(c);

        if (traditional)
            createChildren(c);
    }

    DirectoryNode* taskInfo = create("Taskinfo");

    std::ostringstream os;
    os << taskIdx;
    DirectoryNode* t = taskInfo->create(os.str());

    c = new TimeSignal(task, channels.size());
    channels.push_back(c);
    t->insert(c, "TaskTime");

    c = new StatSignal(task, StatSignal::ExecTime, channels.size());
    channels.push_back(c);
    t->insert(c, "ExecTime");

    c = new StatSignal(task, StatSignal::Period, channels.size());
    channels.push_back(c);
    t->insert(c, "Period");

    c = new StatSignal(task, StatSignal::Overrun, channels.size());
    channels.push_back(c);
    t->insert(c, "Overrun");
}

/////////////////////////////////////////////////////////////////////////////
void Server::createEvents (const PdServ::Config& config)
{
    const PdServ::Main::Events mainEvents = main->getEvents();
    this->events.reserve(mainEvents.size());
    for (PdServ::Main::Events::const_iterator it = mainEvents.begin();
            it != mainEvents.end(); ++it) {

        // Create a name for the event
        std::ostringstream id;
        id << (*it)->id;

        const PdServ::Config eventConfig = config[id.str()];

        Event* e = new Event(*it, eventConfig["level"].toString("debug"));
        this->events.push_back(e);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Server::createParameters(DirectoryNode* baseDir)
{
    const PdServ::Main::ProcessParameters& mainParam = main->getParameters();

    parameters.reserve(parameters.size() + mainParam.size());

    PdServ::Main::ProcessParameters::const_iterator it;
    for (it = mainParam.begin(); it != mainParam.end(); ++it) {
        Parameter* p = new Parameter(*it,
                parameters.size(), (*it)->dtype, (*it)->dim, 0, 0);

        char hidden;
        if (traditional)
            baseDir->traditionalPathInsert(p, (*it)->path, hidden);
        else
            baseDir->pathInsert(p, (*it)->path);

        p->hidden = hidden == 'p' or hidden == 1;

        parameters.push_back(p);
        parameterMap[*it] = p;
            
        if (traditional)
            createChildren(p);
    }

//    if (!root.find<Parameter>("/Taskinfo/Abtastfrequenz")) {
//    }

}

/////////////////////////////////////////////////////////////////////////////
Server::~Server()
{
    for (std::set<Session*>::iterator it = sessions.begin();
            it != sessions.end(); it++)
        delete *it;
}

/////////////////////////////////////////////////////////////////////////////
void Server::initial()
{
    log4cpp::NDC::push("msr");
}

/////////////////////////////////////////////////////////////////////////////
void Server::final()
{
    log4cpp::NDC::pop();
}

/////////////////////////////////////////////////////////////////////////////
void Server::run()
{
    ost::TCPSocket *server = 0;

    ost::tpport_t port = this->port ? this->port : 2345;

    do {
        try {
            server = new ost::TCPSocket(ost::IPV4Address("0.0.0.0"), port);
        }
        catch (ost::Socket *s) {
            if (this->port) {
                log.crit("Could not start on port %i", port);
                throw(s);
            }
            log.notice("Port %i is busy", port);
            port++;
        }
    } while (!server);

    log.notice("Server started on port %i", port);
    while (server->isPendingConnection()) {
        log4cpp::NDC::ContextStack *ndc;
        try {
            ndc = log4cpp::NDC::cloneStack();
            Session *s = new Session(this, server, ndc);

            ost::SemaphoreLock lock(mutex);
            sessions.insert(s);
        }
        catch (ost::Socket *s) {
            log.crit("Socket failure: %s", ::strerror(s->getSystemError()));
            delete ndc;
        }
    }

}

/////////////////////////////////////////////////////////////////////////////
void Server::broadcast(Session *s, const std::string &message)
{
    ost::SemaphoreLock lock(mutex);
    for (std::set<Session*>::iterator it = sessions.begin();
            it != sessions.end(); it++)
        (*it)->broadcast(s, message);
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
            it != sessions.end(); it++) {
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
            it != sessions.end(); it++)
        (*it)->setAIC(p);
}

/////////////////////////////////////////////////////////////////////////////
void Server::parameterChanged(const PdServ::Parameter *mainParam,
        size_t offset, size_t count)
{
    const Parameter *p = find(mainParam);

    ost::SemaphoreLock lock(mutex);
    for (std::set<Session*>::iterator it = sessions.begin();
            it != sessions.end(); it++)
        (*it)->parameterChanged(p);
 
     const Variable::List *children = p->getChildren();
     if (!children)
         return;
 
     for (Variable::List::const_iterator it = children->begin();
             it != children->end(); ++it) {
         const Parameter *child = static_cast<const Parameter*>(*it);
         if (child->offset >= offset 
                 and (child->offset + child->memSize < offset + count)) {
             for (std::set<Session*>::iterator it = sessions.begin();
                     it != sessions.end(); it++)
                 (*it)->parameterChanged(child);
         }
     }
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
void Server::createChildren(Variable* var)
{
    if (var->dim.isScalar() and var->dtype.isPrimary())
        return;

    if (var->dtype.isPrimary())
        createVectorChildren(var, var, std::string(),
                var->dtype, var->dim, 0, 0);
    else
        createCompoundChildren(var, var, var->dtype, var->dim, 0, 0);
}

/////////////////////////////////////////////////////////////////////////////
size_t Server::createVectorChildren (Variable* var,
        DirectoryNode* dir, const std::string& name,
        const PdServ::DataType& dtype,
        const PdServ::DataType::DimType& dim, size_t dimIdx, size_t offset)
{
    if (dimIdx + 1 < dim.size()) {
        if (!name.empty())
            dir = new DirectoryNode(dir, name);

        size_t begin = offset;
        for (size_t i = 0; i < dim[dimIdx]; ++i) {
            std::ostringstream os;
            os << i;
            offset += createVectorChildren(var, dir, os.str(),
                    dtype, dim, dimIdx + 1, offset);
        }

        return offset - begin;
    }

    if (!name.empty())
        dir = createChild(var, dir, name, dtype, dim.back(), offset);

    if (!dim.isScalar()) {
        for (size_t i = 0; i < dim.back(); ++i) {
            std::ostringstream os;
            os << i;
            createChild(var, dir, os.str(), dtype, 1, offset);
            offset += dtype.size;
        }
    }

    return dim.back() * dtype.size;
}

/////////////////////////////////////////////////////////////////////////////
size_t Server::createCompoundChildren (Variable* var,
        DirectoryNode* dir, const PdServ::DataType& dtype,
        const PdServ::DataType::DimType& dim, size_t dimIdx, size_t offset)
{
    if (dimIdx < dim.size() and !dim.isScalar()) {
        size_t begin = offset;
        for (size_t i = 0; i < dim[dimIdx]; ++i) {
            std::ostringstream os;
            os << i;
            offset += createCompoundChildren(
                    var, new DirectoryNode(dir, os.str()),
                    dtype, dim, dimIdx + 1, offset);
        }

        return offset - begin;
    }

    const PdServ::DataType::FieldList& fieldList = dtype.getFieldList();
    PdServ::DataType::FieldList::const_iterator it;
    for (it = fieldList.begin(); it != fieldList.end(); ++it) {

        if ( (*it)->type.isPrimary())
            createVectorChildren(var, dir, (*it)->name,
                    (*it)->type, (*it)->dim, 0, offset + (*it)->offset);
        else
            createCompoundChildren(var,
                    new DirectoryNode(dir, (*it)->name),
                    (*it)->type, (*it)->dim, 0, offset + (*it)->offset);
    }

    return dim.nelem * dtype.size;
}

/////////////////////////////////////////////////////////////////////////////
DirectoryNode* Server::createChild (Variable* var,
        DirectoryNode* dir, const std::string& name,
        const PdServ::DataType& dtype, size_t nelem, size_t offset)
{
    Channel   *c = dynamic_cast<Channel*  >(var);
    Parameter *p = dynamic_cast<Parameter*>(var);

    if (c) {
        c = new Channel(c->signal, channels.size(), dtype,
                PdServ::DataType::DimType(1,&nelem), offset, c);
        channels.push_back(c);
        dir->insert(c, name);

//        log_debug("child %s @ %zu", c->path().c_str(), offset);
        return c;
    }

    if (p) {
        p = new Parameter(p->mainParam, parameters.size(), dtype,
                PdServ::DataType::DimType(1,&nelem), offset, p);
        parameters.push_back(p);
        dir->insert(p, name);

//        log_debug("child %s @ %zu", p->path().c_str(), offset);
        return p;
    }

    return 0;
}
