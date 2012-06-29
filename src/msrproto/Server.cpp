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
Server::Server(const PdServ::Main *main, const PdServ::Config& defaultConfig,
        const PdServ::Config &config):
    main(main),
    log(log4cpp::Category::getInstance(main->name)),
    port(config["port"]),
    traditional( static_cast<unsigned int>( config["traditional"].isTrue()
                ? config["traditional"]
                : defaultConfig["traditional"])),
    mutex(1)
{
    log_debug("traditional=%u", traditional);
    log_debug("port=%u", port);

    const PdServ::Task *primaryTask;
    const std::string prefix(traditional
            ? std::string(1,'/').append(main->name)
            : std::string());
    for (size_t i = 0; i < main->numTasks(); ++i) {
        const PdServ::Task *task = main->getTask(i);

        const PdServ::Task::Signals& signals = task->getSignals();
        for (PdServ::Task::Signals::const_iterator it = signals.begin();
                it != signals.end(); it++)
            root.insert(*it, prefix, traditional);

        std::ostringstream prefix;
        prefix << "/Taskinfo/" << i << '/';
        std::string path;

        if (!i or primaryTask->sampleTime > task->sampleTime)
            primaryTask = task;

        path = prefix.str() + "TaskTime";
        if (!root.find<Channel>(path))
            root.insert(new TimeSignal(task, path), "", traditional);

        path = prefix.str() + "ExecTime";
        if (!root.find<Channel>(path))
            root.insert( new StatSignal(task, path, StatSignal::ExecTime),
                    "", traditional);

        path = prefix.str() + "Period";
        if (!root.find<Channel>(path))
            root.insert( new StatSignal(task, path, StatSignal::Period),
                    "", traditional);

        path = prefix.str() + "Overrun";
        if (!root.find<Channel>(path))
            root.insert( new StatSignal(task, path, StatSignal::Overrun),
                    "", traditional);
    }

//    if (!root.find<Channel>("/Time"))
//        root.insert(new TimeSignal(primaryTask, "/Time"));

    const PdServ::Main::ProcessParameters& mainParam = main->getParameters();
    PdServ::Main::ProcessParameters::const_iterator it;
    for (it = mainParam.begin(); it != mainParam.end(); ++it)
        root.insert(*it, prefix, traditional);

//    if (!root.find<Parameter>("/Taskinfo/Abtastfrequenz")) {
//    }

    start();
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
            sessions.insert(s);
            s->start();
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
void Server::parameterChanged(const PdServ::Parameter *p,
        size_t startIndex, size_t nelem)
{
    ost::SemaphoreLock lock(mutex);
    for (std::set<Session*>::iterator it = sessions.begin();
            it != sessions.end(); it++)
        (*it)->parameterChanged(p, startIndex, nelem);
}

/////////////////////////////////////////////////////////////////////////////
const VariableDirectory& Server::getRoot() const
{
    return root;
}

