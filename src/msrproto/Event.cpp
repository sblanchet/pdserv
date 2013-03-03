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
#include "XmlElement.h"

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Event::Event(const PdServ::Event *e):
    event(e)
{
}

/////////////////////////////////////////////////////////////////////////////
void Event::toXml(ostream::locked& ls, const PdServ::Event* event,
        size_t index, bool state, const struct timespec& t)
{
    XmlElement msg(levelString(event), ls);

    XmlElement::Attribute(msg, "path") << event->path;
    XmlElement::Attribute(msg, "index") << index;
    XmlElement::Attribute(msg, "state") << state;
    XmlElement::Attribute(msg, "time") << t;
    if (event->message)
        XmlElement::Attribute(msg, "message")
            .setEscaped(event->message[index]);
}

/////////////////////////////////////////////////////////////////////////////
const char *Event::levelString(const PdServ::Event *e)
{
    switch (e->priority)
    {
        case PdServ::Event::Emergency:
        case PdServ::Event::Alert:
        case PdServ::Event::Critical:
        case PdServ::Event::Error:
            return "error";
        case PdServ::Event::Warning:
            return "warn";
        case PdServ::Event::Notice:
        case PdServ::Event::Info:
        case PdServ::Event::Debug:
            return "info";
    }

    return "message";
}