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
#include <map>

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

        // The message of the event. %m is replaced by the mapping,
        // Replacements:
        //      %m: value of mapping index->text
        //      %i: index + indexOffset
        std::string message;

        // Offset added to index when generating message to replace %i
        int indexOffset;

        // Index mapping text
        typedef std::map<size_t, std::string> IndexMap;
        IndexMap indexMap;

        std::string formatMessage(size_t index) const;

    private:
};

}

#endif //EVENT_H
