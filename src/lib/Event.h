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

#ifndef LIB_EVENT
#define LIB_EVENT

#include "../Event.h"
#include "pdserv.h"

class Main;
struct timespec;

class Event: public PdServ::Event {
    public:
        Event( Main *main, const char* path,
                const PdServ::Event::Priority& prio, size_t nelem,
                const char **messages);

        const Main * const main;

        /* Pointer into shmem where management data is stored */
        struct EventData *data;

        void set(size_t element, bool state, const timespec *t) const;

    private:
};

#endif //LIB_EVENT
