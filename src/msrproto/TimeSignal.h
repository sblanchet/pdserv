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

#ifndef MSRTIMESIGNAL_H
#define MSRTIMESIGNAL_H

#include "../Signal.h"

namespace PdServ {
    class Session;
    class Task;
    class SessionTaskData;
}

namespace MsrProto {

class Server;

class TimeSignal: public PdServ::Signal {
    public:
        TimeSignal(const PdServ::Task *task, const std::string& path);

        const PdServ::Task * const task;

    private:
        // Reimplemented from PdServ::Signal
        void subscribe(PdServ::Session *) const;
        void unsubscribe(PdServ::Session *) const;
        double poll(const PdServ::Session *s,
                void *buf, struct timespec *t) const;
        const void *getValue(const PdServ::SessionTaskData*) const;
        void getValue(const PdServ::Session*, void*, struct timespec*) const;
};

}

#endif //MSRTIMESIGNAL_H
