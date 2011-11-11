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

#include "config.h"

#include "Server.h"
#include "Channel.h"
#include "TimeSignal.h"
#include "Parameter.h"
#include "Directory.h"
#include "../Main.h"
#include "../Task.h"
#include "../Signal.h"
#include "../Parameter.h"
#include <cc++/socketport.h>

#include <algorithm>

#include "Session.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Server::Server(const PdServ::Main *main, int argc, const char **argv):
    main(main), mutex(1)
{
    bool traditional = 1;

    root = new VariableDirectory;

    const PdServ::Main::Signals& mainSignals = main->getSignals();
    for (PdServ::Main::Signals::const_iterator it = mainSignals.begin();
            it != mainSignals.end(); it++) {

        if (traditional) {
            for (unsigned i = 0; i < (*it)->nelem; i++)
                root->insert(*it, i, 1);
        }
        else {
            // New matrix channel
            root->insert(*it);
        }
    }

    if (!main->findVariable("/Time")) {
        double t = main->getTask(0U)->sampleTime;
        for (size_t i = 1; i < main->numTasks(); ++i)
            t = std::min(t, main->getTask(i)->sampleTime);

        time = new TimeSignal(this, t);
        root->insert(time);
    }
    else
        time = 0;

    const PdServ::Main::Parameters& mainParameters = main->getParameters();
    for (PdServ::Main::Parameters::const_iterator it = mainParameters.begin();
            it != mainParameters.end(); it++) {
        size_t nelem = (*it)->nelem;
        const size_t *dim = (*it)->getDim();
        size_t vectorLen = dim[(*it)->ndims-1];

        parameterIndexMap[*it] = getRoot().getParameters().size();
        if (traditional) {
            bool dep = vectorLen > 1;
            for (unsigned i = 0; i < nelem; i++) {
                if (vectorLen > 1 && !(i % vectorLen)) {
                    // New row parameter
                    root->insert(*it, i, vectorLen);
                }
                root->insert(*it, i, 1, dep);
            }
        }
        else {
            // New matrix parameter
            root->insert(*it);
        }
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
void Server::broadcast(Session *s, const MsrXml::Element &element)
{
    struct timespec ts;
    main->gettime(&ts);

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
        stats.push_back((*it)->getStatistics());
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

