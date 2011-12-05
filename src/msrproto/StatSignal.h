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

#ifndef MSRSTATSIGNAL_H
#define MSRSTATSIGNAL_H

#include "../Signal.h"
#include <stdint.h>

namespace PdServ {
    class Session;
    class Task;
    class SessionTaskData;
}

namespace MsrProto {

class Server;

class StatSignal: public PdServ::Signal {
    public:
        enum Type {ExecTime, Period, Overrun};

        StatSignal(const PdServ::Task *task,
                const std::string& path, Type type);

        const PdServ::Task * const task;

    private:
        const Type type;

        uint32_t getValue( const PdServ::Session *session,
                struct timespec *t) const;

        // Reimplemented from PdServ::Signal
        void subscribe(PdServ::Session *) const;
        void unsubscribe(PdServ::Session *) const;
        double poll(const PdServ::Session *s,
                void *buf, struct timespec *t) const;
        const void *getValue(const PdServ::SessionTaskData*) const;
        void getValue(const PdServ::Session*, void*, struct timespec*) const;
};

}

#endif //MSRSTATSIGNAL_H
