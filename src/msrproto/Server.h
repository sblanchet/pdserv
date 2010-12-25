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

#ifndef MSRSERVER_H
#define MSRSERVER_H

#include <set>
#include <map>
#include <vector>
#include <list>
#include <cc++/thread.h>

#include "../SessionStatistics.h"

namespace HRTLab {
    class Main;
    class Parameter;
}

namespace MsrXml {
    class Element;
}

namespace MsrProto {

class Session;
class Channel;
class Parameter;

class Server:public ost::Thread {
    public:
        Server(HRTLab::Main *main, bool traditional);
        ~Server();

        void broadcast(Session *s, const MsrXml::Element&);

        void parametersChanged(const HRTLab::Parameter* const *, size_t n);

        void sessionClosed(Session *s);

        void getSessionStatistics(
                std::list<HRTLab::SessionStatistics>& stats) const;

        typedef std::vector<const Channel*> Channels;
        const Channels& getChannels() const;
        const Channel* getChannel(const std::string&) const;
        const Channel* getChannel(unsigned int) const;

        typedef std::vector<const Parameter*> Parameters;
        const Parameters& getParameters() const;
        const Parameters& getParameters(const HRTLab::Parameter *p) const;
        const Parameter* getParameter(const std::string&) const;
        const Parameter* getParameter(unsigned int) const;

    private:

        HRTLab::Main * const main;
        std::set<Session*> sessions;

        Channels channel;
        typedef std::map<const std::string, Channel*> ChannelMap;
        ChannelMap channelMap;

        Parameters parameter;
        typedef std::map<const std::string, Parameter*> ParameterMap;
        ParameterMap parameterMap;

        std::map<const HRTLab::Parameter*, Parameters> mainParameterMap;

        mutable ost::Semaphore mutex;

        void run();

};

}
#endif //MSRSERVER_H
