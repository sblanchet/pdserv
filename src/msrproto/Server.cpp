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

    std::list<const Channel*> channelList;
    for (HRTLab::Main::Signals::const_iterator it = mainSignals.begin();
            it != mainSignals.end(); it++) {
        root.insert(*it, channelList, traditional);
    }
    channel.resize(channelList.size());
    std::copy(channelList.begin(), channelList.end(), channel.begin());

    if (!main->getSignal("/Time"));

    std::list<const Parameter*> parameterList;
    for (HRTLab::Main::Parameters::const_iterator it = mainParameters.begin();
            it != mainParameters.end(); it++) {
        root.insert(*it, parameterList, traditional);
    }
    parameter.resize(parameterList.size());
    std::copy(parameterList.begin(), parameterList.end(), parameter.begin());

//            const HRTLab::Parameter *mainParam = *it;
//            size_t vectorLen = mainParam->getDim()[mainParam->ndims - 1];
//            Parameter *p = 0;
//            mainParameterMap[mainParam].reserve(1 + (*it)->nelem / vectorLen);
//            for (size_t i = 0; i < (*it)->nelem; i += vectorLen) {
//                p = new Parameter(mainParam, idx, vectorLen, i);
//                if (root.insert(*it,
//                            makeExtension(*it, i, vectorLen).c_str(),
//                            0, p)) {
//                    parameter.push_back(p);
//                    mainParameterMap[mainParam].push_back(p);
//                    idx++;
//                }
//                else
//                    delete p;
//
//                for (size_t j = i; j < i + vectorLen; j++) {
//                    p = new Parameter(mainParam, idx, 1, j);
//                    if (root.insert(*it, makeExtension(*it, j).c_str(), 0, p)) {
//                        parameter.push_back(p);
//                        mainParameterMap[mainParam].push_back(p);
//                        idx++;
//                    }
//                    else
//                        delete p;
//                }
//            }
//        }
//
//        if (!main->getParameter("/Taskinfo/Abtastfrequenz"));
//    }
//    else {
//        size_t idx = 0;
//
//        channel.reserve(mainSignals.size());
//        for (HRTLab::Main::Signals::const_iterator it = mainSignals.begin();
//                it != mainSignals.end(); it++) {
//            Channel *c = new Channel(*it, idx, 0);
//            if ( root.insert(*it, "", c, 0)) {
//                channel.push_back(c);
//                idx++;
//            }
//            else
//                delete c;
//        }
//
//
//        idx = 0;
//        parameter.reserve(mainParameters.size());
//        HRTLab::Main::Parameters::const_iterator it;
//        for (it = mainParameters.begin(); it != mainParameters.end(); it++) {
//            Parameter *p = new Parameter(*it, idx, (*it)->nelem);
//            if ( root.insert(*it, "", 0, 0)) {
//                parameter.push_back(p);
//                idx++;
//            }
//            else
//                delete p;
//        }
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
    return parameter[idx];
}

