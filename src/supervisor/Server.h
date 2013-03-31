/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdserv library.
 *
 *  The pdserv library is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or (at
 *  your option) any later version.
 *
 *  The pdserv library is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with the pdserv library. If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#ifndef SUPERVISOR_SERVER_H
#define SUPERVISOR_SERVER_H

#include <cc++/thread.h>

#include "../Config.h"
#include "../Session.h"

namespace PdServ {
    class Main;
    class Config;
}

namespace Supervisor {

class Session;
class Event;

class Server: public ost::Thread, private PdServ::Session {
    public:
        Server(const PdServ::Main *main, const PdServ::Config &config);
        ~Server();

    private:
        const PdServ::Main * const main;

        mutable ost::Semaphore mutex;

        // Reimplemented from ost::Thread
        void run();
};

}

#endif //SUPERVISOR_SERVER_H
