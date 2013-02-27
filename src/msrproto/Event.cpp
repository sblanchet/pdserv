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

#include <sstream>
#include <libintl.h>

#include "Event.h"
#include "XmlElement.h"
#include "../Config.h"

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Event::Event(const PdServ::Event *e, const std::string& level):
    event(e), level(level)
{
}

/////////////////////////////////////////////////////////////////////////////
void Event::toXml(ostream::locked& ls,
        size_t index, bool state, const struct timespec& t) const
{
    XmlElement msg(level.c_str(), ls);

    XmlElement::Attribute(msg, "id") << event->id;
    XmlElement::Attribute(msg, "index") << index;
    XmlElement::Attribute(msg, "state") << state;
    XmlElement::Attribute(msg, "time") << t;
    if (event->message)
        XmlElement::Attribute(msg, "message")
            .setEscaped(event->message[index]);
}
