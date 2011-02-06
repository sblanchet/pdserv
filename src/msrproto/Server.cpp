/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdcomserv package.
 *
 *  pdcomserv is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pdcomserv is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pdcomserv. See COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include "config.h"

#include "Server.h"
#include "Channel.h"
#include "Parameter.h"
#include "../Main.h"
#include "../Signal.h"
#include "../Parameter.h"
#include <cc++/socketport.h>

#include "Session.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Server::Server(HRTLab::Main *main, bool traditional):
    main(main), mutex(1)
{
    const HRTLab::Main::Signals& mainSignals = main->getSignals();
    const HRTLab::Main::Parameters& mainParameters = main->getParameters();

    //FIXME
    traditional = 1;

    for (HRTLab::Main::Signals::const_iterator it = mainSignals.begin();
            it != mainSignals.end(); it++) {
        size_t nelem = (*it)->nelem;
        Channel *c;

        if (traditional) {
            for (unsigned i = 0; i < nelem; i++) {
                DirectoryNode *dir = root.mkdir(*it, i, 0);
                if (dir) {
                    c = new Channel( dir, *it, channel.size(), i, 1);
                    dir->insert(c);
                    channel.push_back(c);
                    cout << "Insert channel " << c->path() << endl;
                }
            }
        }
        else {
            // New matrix channel
            DirectoryNode *dir = root.mkdir(*it);
            if (dir) {
                c = new Channel( dir, *it, channel.size(), 0, nelem);
                dir->insert(c);
                channel.push_back(c); cout << "Insert matrix" << c->path() << endl;
            }
        }
    }

//    if (!main->getSignal("/Time"));

    for (HRTLab::Main::Parameters::const_iterator it = mainParameters.begin();
            it != mainParameters.end(); it++) {
        size_t nelem = (*it)->nelem;
        const size_t *dim = (*it)->getDim();
        size_t vectorLen = dim[(*it)->ndims-1];
        Parameter *p;

        parameterIndexMap[*it] = parameter.size();
        if (traditional) {
            for (unsigned i = 0; i < nelem; i++) {
                if (vectorLen > 1 && !(i % vectorLen)) {
                    // New row parameter
                    DirectoryNode *dir = root.mkdir(*it, i, 1);
                    if (dir) {
                        p = new Parameter(dir, 0, *it, parameter.size(),
                                vectorLen, i);
                        dir->insert(p);
                        parameter.push_back(p);
                        cout << "Insert vector" << p->path() << endl;
                    }
                }
                DirectoryNode *dir = root.mkdir(*it, i, 0);
                if (dir) {
                    p = new Parameter( dir, vectorLen > 1,
                            *it, parameter.size(), 1, i);
                    dir->insert(p);
                    parameter.push_back(p);
                    cout << "Insert element" << p->path() << endl;
                }
            }
        }
        else {
            // New matrix parameter
            DirectoryNode *dir = root.mkdir(*it);
            if (dir) {
                p = new Parameter(
                        dir, 0, *it, parameter.size(), nelem, 0);
                dir->insert(p);
                parameter.push_back(p);
                cout << "Insert matrix" << p->path() << endl;
            }
        }
    }

//    if (!main->getParameter("/Taskinfo/Abtastfrequenz"));
}

/////////////////////////////////////////////////////////////////////////////
Server::~Server()
{
    for (std::set<Session*>::iterator it = sessions.begin();
            it != sessions.end(); it++)
        delete *it;
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
        std::list<HRTLab::SessionStatistics>& stats) const
{
    ost::SemaphoreLock lock(mutex);
    for (std::set<Session*>::iterator it = sessions.begin();
            it != sessions.end(); it++)
        stats.push_back((*it)->getStatistics());
}

/////////////////////////////////////////////////////////////////////////////
void Server::parameterChanged(const HRTLab::Parameter *p, 
        size_t startIndex, size_t nelem)
{
    ost::SemaphoreLock lock(mutex);
    for (std::set<Session*>::iterator it = sessions.begin();
            it != sessions.end(); it++)
        (*it)->parameterChanged(p, startIndex, nelem);
}

/////////////////////////////////////////////////////////////////////////////
size_t Server::getParameterIndex(const HRTLab::Parameter *p) const
{
    return parameterIndexMap.find(p)->second;
}

/////////////////////////////////////////////////////////////////////////////
const DirectoryNode& Server::getRoot() const
{
    return root;
}

/////////////////////////////////////////////////////////////////////////////
const Server::Channels& Server::getChannels() const
{
    return channel;
}

/////////////////////////////////////////////////////////////////////////////
const Channel* Server::getChannel(unsigned int idx) const
{
    return idx < channel.size() ? channel[idx] : 0;
}

/////////////////////////////////////////////////////////////////////////////
const Server::Parameters& Server::getParameters() const
{
    return parameter;
}

// /////////////////////////////////////////////////////////////////////////////
// const Server::Parameters& Server::getParameters(const HRTLab::Parameter *p) const
// {
// //    return mainParameterMap.find(p)->second;
// }

/////////////////////////////////////////////////////////////////////////////
const Parameter* Server::getParameter(unsigned int idx) const
{
    return idx < parameter.size() ? parameter[idx] : 0;
}

