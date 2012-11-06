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
        Event( Main *main, size_t idx,
                const PdServ::Event::Priority& prio, int id, size_t nelem);

        const Main * const main;

        struct EventData *data;

        void setClear(pdserv_event_clear_t clear, void *priv_data);
        void set(size_t element, bool state, const timespec *t) const;

    private:

        pdserv_event_clear_t clear_func;
        void *priv_data;
};

#endif //LIB_EVENT
