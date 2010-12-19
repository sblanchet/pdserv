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
#include "../Main.h"
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
Server::Server(HRTLab::Main *main): main(main), mutex(1)
{
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
void Server::parametersChanged(const HRTLab::Parameter * const *p, size_t n)
{
    ost::SemaphoreLock lock(mutex);
    for (std::set<Session*>::iterator it = sessions.begin();
            it != sessions.end(); it++)
        (*it)->parametersChanged(p, n);
}
