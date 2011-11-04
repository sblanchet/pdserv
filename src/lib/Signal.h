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

#ifndef LIB_SIGNAL
#define LIB_SIGNAL

#include <set>
#include <cc++/thread.h>

#include "../Signal.h"

namespace PdServ {
    class Session;
}

class Task;

class Signal: public PdServ::Signal {
    public:
        Signal( Task *task,
                unsigned int decimation,
                const char *path,
                enum si_datatype_t dtype,
                const void *addr,
                unsigned int ndims = 1,
                const unsigned int *dim = 0);

        const char * const addr;
        const Task * const task;

        const size_t subscriptionIndex;

        mutable size_t copyListPosition;
        mutable char *pollDest;

    private:
        mutable ost::Semaphore mutex;

        static const size_t dataTypeIndex[PdServ::Variable::maxWidth+1];

        typedef std::set<const PdServ::Session*> SessionSet;
        mutable SessionSet sessions;


        // Reimplemented from PdServ::Signal
        void subscribe(PdServ::Session *) const;
        void unsubscribe(PdServ::Session *) const;
        double sampleTime() const;
        double poll(const PdServ::Session *s, char *buf) const;

        // Reimplemented from PdServ::Variable
        void getValue(PdServ::Session*, char *, struct timespec * = 0) const;
};

#endif //LIB_SIGNAL
