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
        Event(const PdServ::Event *s, const std::string& level);

        const PdServ::Event *event;
        void toXml(ostream::locked& ls,
                size_t index, bool state, const struct timespec& t) const;

    private:
        const std::string level;
};

}

#endif //MSREVENT_H
