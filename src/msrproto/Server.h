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

namespace log4cpp {
    class Category;
}

namespace PdServ {
    class Main;
    class Parameter;
}

namespace MsrProto {

class Session;
class Channel;
class TimeSignal;
class Parameter;
class XmlElement;
class VariableDirectory;

class Server: public ost::Thread {
    public:
        Server(const PdServ::Main *main, int port);
        ~Server();

        void broadcast(Session *s, const std::string&);

        void setAic(const Parameter*);
        void parameterChanged(const PdServ::Parameter*,
                size_t startIndex, size_t n);

        void sessionClosed(Session *s);

        const VariableDirectory& getRoot() const;

        void getSessionStatistics(
                std::list<PdServ::SessionStatistics>& stats) const;

        const PdServ::Main * const main;
        log4cpp::Category &log;

    private:
        std::set<Session*> sessions;
        int port;

        VariableDirectory *root;

        mutable ost::Semaphore mutex;

        // Reimplemented from ost::Thread
        void initial();
        void run();
        void final();
};

}
#endif //MSRSERVER_H
