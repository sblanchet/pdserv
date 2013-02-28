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

#ifndef MSREVENT_H
#define MSREVENT_H

#include "OStream.h"
#include "../Event.h"

namespace MsrProto {

class Event {
    public:
        Event(const PdServ::Event *s);

        const PdServ::Event *event;
        static void toXml(ostream::locked& ls, const PdServ::Event* event,
                size_t index, bool state, const struct timespec& t);

    private:
        static const char* levelString(const PdServ::Event *e);
};

}

#endif //MSREVENT_H
