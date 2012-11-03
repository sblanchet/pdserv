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

#ifndef EVENT_H
#define EVENT_H

#include <string>

namespace PdServ {

class Session;
class SessionTask;
class Config;

class Event {
    public:
        Event(size_t index, int id, size_t nelem);

        ~Event();

        const size_t index;

        const int id;
        const size_t nelem;

        std::string formatMessage(const Config& config, size_t index) const;
//        void set(int level, size_t element, const timespec *t) const;

    private:
};

}

#endif //EVENT_H
