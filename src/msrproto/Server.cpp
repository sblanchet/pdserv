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
    if (1 || traditional) {
        // Stop at variables with <hide>; continue at <unhide>
        size_t idx = 0;
        for (HRTLab::Main::Signals::const_iterator it = mainSignals.begin();
                it != mainSignals.end(); it++)
            idx += (*it)->nelem;

        channel.resize(idx);
        idx = 0;
        for (HRTLab::Main::Signals::const_iterator it = mainSignals.begin();
                it != mainSignals.end(); it++) {
            cout << __PRETTY_FUNCTION__ << (*it)->path << endl;
            for (size_t i = 0; i < (*it)->nelem; idx++, i++) {
                Channel *c = new Channel(*it, idx, i);
                channel[idx] = c;
                channelMap[c->path()] = c;
            }
        }

        if (!main->getSignal("/Time"));

        idx = 0;
        for (HRTLab::Main::Parameters::const_iterator it = mainParameters.begin();
                it != mainParameters.end(); it++) {
            idx += (*it)->nelem
                + ((*it)->nelem > 1
                        ? (*it)->nelem / (*it)->getDim()[(*it)->ndims - 1]
                        : 0);
        }


        parameter.resize(idx);
        idx = 0;
        for (HRTLab::Main::Parameters::const_iterator it = mainParameters.begin();
                it != mainParameters.end(); it++) {
            cout << __PRETTY_FUNCTION__ << (*it)->path << endl;
            const HRTLab::Parameter *mainParam = *it;
            size_t vectorLen = mainParam->getDim()[mainParam->ndims - 1];
            Parameter *p = 0;
            for (size_t i = 0; i < (*it)->nelem; i++) {
                if (!(i % vectorLen)) {
                    p = new Parameter(mainParam, idx, i, vectorLen);
                    parameter[idx] = p;
                    mainParameterMap[mainParam].push_back(p);
                    parameterMap[p->path()] = p;
                    idx++;
                }

                if (vectorLen > 1) {
                    p = new Parameter(mainParam, idx, i);
                    parameter[idx] = p;
                    mainParameterMap[mainParam].push_back(p);
                    parameterMap[p->path()] = p;
                    idx++;
                }
            }
        }

        if (!main->getParameter("/Taskinfo/Abtastfrequenz"));
    }
    else {
        size_t idx = 0;

        channel.resize(mainSignals.size());
        for (HRTLab::Main::Signals::const_iterator it = mainSignals.begin();
                it != mainSignals.end(); idx++, it++) {
            channel[idx] = new Channel(*it, idx);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
Server::~Server()
{
    for (std::set<Session*>::iterator it = sessions.begin();
            it != sessions.end(); it++)
        delete *it;

    for (size_t i = 0; i < channel.size(); i++)
        delete channel[i];
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

/////////////////////////////////////////////////////////////////////////////
const Server::Channels& Server::getChannels() const
{
    return channel;
}

/////////////////////////////////////////////////////////////////////////////
const Channel* Server::getChannel(const std::string& path) const
{
    ChannelMap::const_iterator it = channelMap.find(path);
    return it != channelMap.end() ? it->second : 0;
}

/////////////////////////////////////////////////////////////////////////////
const Channel* Server::getChannel(unsigned int idx) const
{
    return channel[idx];
}

/////////////////////////////////////////////////////////////////////////////
const Server::Parameters& Server::getParameters() const
{
    return parameter;
}

/////////////////////////////////////////////////////////////////////////////
const Server::Parameters& Server::getParameters(const HRTLab::Parameter *p) const
{
    return mainParameterMap.find(p)->second;
}

/////////////////////////////////////////////////////////////////////////////
const Parameter* Server::getParameter(const std::string& path) const
{
    return parameterMap.find(path)->second;
}

/////////////////////////////////////////////////////////////////////////////
const Parameter* Server::getParameter(unsigned int idx) const
{
    return parameter[idx];
}

