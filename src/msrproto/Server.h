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

#ifndef MSRSERVER_H
#define MSRSERVER_H

#include <set>
#include <map>
#include <vector>
#include <list>
#include <cc++/thread.h>

#include "../SessionStatistics.h"
#include "Directory.h"

namespace PdServ {
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

class Server: public ost::Thread {
    public:
        Server(PdServ::Main *main, int argc, const char **argv);
        ~Server();

        void broadcast(Session *s, const MsrXml::Element&);

        void parameterChanged(const PdServ::Parameter*,
                size_t startIndex, size_t n);

        void sessionClosed(Session *s);

        const DirectoryNode& getRoot() const;

        void getSessionStatistics(
                std::list<PdServ::SessionStatistics>& stats) const;

        typedef std::vector<const Channel*> Channels;
        typedef std::vector<const Parameter*> Parameters;
        const Channels& getChannels() const;
        const Channel* getChannel(unsigned int) const;
        const Parameters& getParameters() const;
        size_t getParameterIndex(const PdServ::Parameter *) const;
        const Parameters& getParameters(const PdServ::Parameter *p) const;
        const Parameter* getParameter(unsigned int) const;

    private:
        PdServ::Main * const main;
        std::set<Session*> sessions;

        DirectoryNode root;

        Channels channel;
        Parameters parameter;

        typedef std::map<const PdServ::Parameter*, size_t> ParameterMap;
        ParameterMap parameterIndexMap;

        mutable ost::Semaphore mutex;

        void run();

};

}
#endif //MSRSERVER_H
