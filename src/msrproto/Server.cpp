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
Server::Server(const PdServ::Main *main): main(main), mutex(1)
{
    root = new VariableDirectory;

    const PdServ::Task *primaryTask;
    for (unsigned int i = 0; i < main->numTasks(); ++i) {
        const PdServ::Task *task = main->getTask(i);

        const PdServ::Task::Signals& signals = task->getSignals();
        for (PdServ::Task::Signals::const_iterator it = signals.begin();
                it != signals.end(); it++) {

            root->insert(*it);
        }

        std::ostringstream prefix;
        prefix << "/Taskinfo/" << i << '/';
        std::string path;
        debug() << prefix;

        if (!i or primaryTask->sampleTime > task->sampleTime)
            primaryTask = task;

        path = prefix.str() + "TaskTime";
        if (!root->find<Channel>(path))
            root->insert(new TimeSignal(task, path));

        path = prefix.str() + "ExecTime";
        if (!root->find<Channel>(path))
            root->insert( new StatSignal(task, path, StatSignal::ExecTime));

        path = prefix.str() + "Period";
        if (!root->find<Channel>(path))
            root->insert( new StatSignal(task, path, StatSignal::Period));

        path = prefix.str() + "Overrun";
        if (!root->find<Channel>(path)) {
            debug() << "insert" << path;
            root->insert( new StatSignal(task, path, StatSignal::Overrun));
        }
        else {
            debug() << "cant find" << path;
        }

    }

    if (!main->findVariable("/Time"))
        root->insert(new TimeSignal(primaryTask, "/Time"));

    const PdServ::Main::Parameters& mainParameters = main->getParameters();
    for (PdServ::Main::Parameters::const_iterator it = mainParameters.begin();
            it != mainParameters.end(); it++) {

        root->insert(*it);
    }

    if (!main->findVariable("/Taskinfo/Abtastfrequenz")) {
    }

    start();
}

/////////////////////////////////////////////////////////////////////////////
Server::~Server()
{
    debug();
    for (std::set<Session*>::iterator it = sessions.begin();
            it != sessions.end(); it++)
        delete *it;
}

// class TSession: public ost::TCPSession {
//     public:
//         TSession(Server *server, ost::TCPSocket &socket):
//             TCPSession(socket) {
//                 detach();
//             }
//         ~TSession() {
//             debug();
//         }
// 
//     private:
//         void run() {
//             *this << "hallo\r\n" << std::flush;
//         }
// 
//         void _final() {
//             disconnect();
// //            delete this;
//         }
// };

/////////////////////////////////////////////////////////////////////////////
void Server::run()
{
    ost::TCPSocket server(ost::IPV4Address("0.0.0.0"), 2345);

    while (server.isPendingConnection()) {
        try {
            Session *s = new Session(this, server);
            sessions.insert(s);
            s->start();
        }
        catch (ost::Socket *s) {
            // FIXME Log this
            debug() << s->getErrorNumber() << s->getSystemError();
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
    return *root;
}

