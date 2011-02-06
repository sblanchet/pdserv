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

#ifndef LIB_SIGNAL
#define LIB_SIGNAL

#include <set>
#include <cc++/thread.h>

#include "../Signal.h"

namespace HRTLab {
    class Session;
}

class Task;
class Main;

class Signal: public HRTLab::Signal {
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

        typedef std::set<const HRTLab::Session*> SessionSet;
        mutable SessionSet sessions;

    private:
        static const size_t dataTypeIndex[HRTLab::Variable::maxWidth+1];

        // Reimplemented from HRTLab::Signal
        void subscribe(HRTLab::Session *) const;
        void unsubscribe(HRTLab::Session *) const;
        double sampleTime() const;

        // Reimplemented from HRTLab::Variable
        void getValue(char *, struct timespec * = 0) const;
};

#endif //LIB_SIGNAL
