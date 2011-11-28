/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdserv package.
 *
 *  pdserv is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pdserv is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pdserv. See COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include "Server.h"
#include "Channel.h"
#include "TimeSignal.h"
#include "StatSignal.h"
#include "Parameter.h"
#include "Directory.h"
#include "Session.h"
#include "../Main.h"
#include "../Task.h"
#include "../Signal.h"
#include "../Parameter.h"
#include "../Debug.h"

#include <cc++/socketport.h>
#include <algorithm>

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Server::Server(const PdServ::Main *main, int argc, const char **argv):
    main(main), mutex(1)
{
    traditional = 1;

    root = new VariableDirectory;

    const PdServ::Main::Signals& mainSignals = main->getSignals();
    for (PdServ::Main::Signals::const_iterator it = mainSignals.begin();
            it != mainSignals.end(); it++) {

        root->insert(*it, traditional);
    }

    const PdServ::Task *primaryTask;
    TimeSignal *primaryTaskTimeSignal = 0;
    for (unsigned int i = 0; i < main->numTasks(); ++i) {
        const PdServ::Task *task = main->getTask(i);

        std::ostringstream prefix;
        prefix << "/Taskinfo/" << i << '/';
        std::string path;
        debug() << prefix;

        if (!i or primaryTask->sampleTime > task->sampleTime)
            primaryTask = task;

        path = prefix.str() + "TaskTime";
        if (!root->findChannel(path)) {
            TimeSignal *t = new TimeSignal(task, path);
            root->insert(t, traditional);
            if (task == primaryTask)
                primaryTaskTimeSignal = t;
        }

        path = prefix.str() + "ExecTime";
        if (!root->findChannel(path))
            root->insert(
                    new StatSignal(task, path, StatSignal::ExecTime),
                    traditional);

        path = prefix.str() + "Period";
        if (!root->findChannel(path))
            root->insert(
                    new StatSignal(task, path, StatSignal::Period),
                    traditional);

        path = prefix.str() + "Overrun";
        if (!root->findChannel(path)) {
            debug() << "insert" << path;
            root->insert(
                    new StatSignal(task, path, StatSignal::Overrun),
                    traditional);
        }
        else {
            debug() << "cant find" << path;
        }

    }

    if (!main->findVariable("/Time"))
        root->insert(new TimeSignal(primaryTask, "/Time"), traditional);

    const PdServ::Main::Parameters& mainParameters = main->getParameters();
    for (PdServ::Main::Parameters::const_iterator it = mainParameters.begin();
            it != mainParameters.end(); it++) {

        root->insert(*it, traditional);
    }

    if (!main->findVariable("/Taskinfo/Abtastfrequenz")) {
    }
}

/////////////////////////////////////////////////////////////////////////////
Server::~Server()
{
    for (std::set<Session*>::iterator it = sessions.begin();
            it != sessions.end(); it++)
        delete *it;

//    delete time;
}

/////////////////////////////////////////////////////////////////////////////
void Server::run()
{
    ost::SocketService svc;
    ost::TCPSocket socket(ost::IPV4Address("0.0.0.0"), 2345);
    while (true) {
        Session *s = new Session(this, &svc, socket, main);

        ost::SemaphoreLock lock(mutex);
        sessions.insert(s);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Server::broadcast(Session *s, const XmlElement &element)
{
    ost::SemaphoreLock lock(mutex);
    for (std::set<Session*>::iterator it = sessions.begin();
            it != sessions.end(); it++)
        (*it)->broadcast(s, element);
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
            it != sessions.end(); it++)
        (*it)->getSessionStatistics(stats);
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
    return *root;
}

