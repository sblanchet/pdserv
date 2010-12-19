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
#include <cc++/thread.h>

#include "Session.h"
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

class Server:public ost::Thread {
    public:
        Server(HRTLab::Main *main);
        ~Server();

        void broadcast(Session *s, const MsrXml::Element&);

        void parametersChanged(const HRTLab::Parameter* const *, size_t n);

        void sessionClosed(Session *s);

        void getSessionStatistics(
                std::list<HRTLab::SessionStatistics>& stats) const;

    private:

        HRTLab::Main * const main;
        std::set<Session*> sessions;

        mutable ost::Semaphore mutex;

        void run();

};

}
#endif //MSRSERVER_H
