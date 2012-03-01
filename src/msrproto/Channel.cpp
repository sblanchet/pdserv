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

#include "Channel.h"
#include "Directory.h"
#include "XmlElement.h"
#include "../Signal.h"

#include <sstream>

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Channel::Channel(const Server *server, const PdServ::Signal *s,
        unsigned int channelIndex, unsigned int elementIndex):
    Variable(server, s, channelIndex, elementIndex),
    signal(s)
{

    //cout << __PRETTY_FUNCTION__ << index << endl;
//    cout << path() << ' ' << index << ' ' << sigOffset << endl;

}

/////////////////////////////////////////////////////////////////////////////
Channel::~Channel()
{
}

/////////////////////////////////////////////////////////////////////////////
void Channel::setXmlAttributes( XmlElement &element,
        bool shortReply, const char *data, size_t precision) const
{
    // <channel name="/lan/World Time" alias="" index="0" typ="TDBL"
    //   datasize="8" bufsize="500" HZ="50" unit="" value="1283134199.93743"/>
    double freq = 1.0 / signal->sampleTime / signal->decimation;

    // The MSR protocoll wants a bufsize, the maximum number of
    // values that can be retraced. This artificial limitation does
    // not exist any more. Instead, choose a buffer size so that
    // at a maximum of 10 seconds has to be stored.
    size_t bufsize = 10 * std::max( 1U, (unsigned int)(freq + 0.5));

    setAttributes(element, shortReply);

    if (shortReply)
        return;

    // bufsize=
    XmlElement::Attribute(element, "bufsize") << bufsize;
    XmlElement::Attribute(element, "HZ") << freq;

    if (data)
        XmlElement::Attribute(element, "value")
            .csv( this, data + elementIndex * signal->elemSize, 1, precision);
}
