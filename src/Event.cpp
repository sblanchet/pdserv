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

#include "Event.h"
#include "Config.h"

#include <sstream>

using namespace PdServ;

//////////////////////////////////////////////////////////////////////
Event::Event(size_t idx, int id, size_t nelem):
    index(idx),
    id(id),
    nelem(nelem)
{
}

//////////////////////////////////////////////////////////////////////
Event::~Event()
{
}

//////////////////////////////////////////////////////////////////////
std::string Event::formatMessage(const Config& config, size_t index) const
{
    std::ostringstream os;
    os << index;
    std::string text =
        config["indexmapping"][os.str()].toString();
    if (text.empty()) {
        std::ostringstream os;
        os << index + config["indexoffset"].toInt();
        text = os.str();
    }

    std::string message = config["message"].toString().c_str();
    size_t i = message.find("%m");
    if (i != message.npos)
        message.replace(i, 2, text);

    return message;
}
