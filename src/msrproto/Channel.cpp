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
#include "XmlElement.h"
#include "../Signal.h"
#include "../DataType.h"

#include <sstream>

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Channel::Channel(const PdServ::Signal *s,
        const std::string& name, DirectoryNode* dir,
        unsigned int channelIndex, bool traditional):
    Variable(s, name, dir, channelIndex),
    signal(s)
{
    //cout << __PRETTY_FUNCTION__ << index << endl;
//    log_debug("new var %s idx=%zu size=%zu, nelem=%zu", path().c_str(),
//            variableIndex, dtype.size, dim.nelem);
    //traditional = true;
    createChildren(traditional);
}

/////////////////////////////////////////////////////////////////////////////
Channel::Channel(const Channel *parent, const std::string& name,
        DirectoryNode *dir, const PdServ::DataType& dtype,
        size_t nelem, size_t offset):
    Variable(parent, name, dir, dtype, nelem, offset),
    signal(parent->signal)
{
//    log_debug("new var %s idx=%zu size=%zu, nelem=%zu", path().c_str(),
//            variableIndex, dtype.size, dim.nelem);
    createChildren(true);
}

/////////////////////////////////////////////////////////////////////////////
const Variable* Channel::createChild(DirectoryNode* dir,
                const std::string& name,
                const PdServ::DataType& dtype, size_t nelem, size_t offset)
{
    return new Channel(this, name, dir, dtype, nelem, offset);
}

/////////////////////////////////////////////////////////////////////////////
void Channel::setXmlAttributes( XmlElement &element,
        bool shortReply, const char *data, size_t precision) const
{
    setAttributes(element, shortReply);

    if (!shortReply) {
        // <channel name="/lan/World Time" alias="" index="0" typ="TDBL"
        //   datasize="8" bufsize="500" HZ="50" unit="" value="1283134199.93743"/>
        double freq = 1.0 / signal->sampleTime / signal->decimation;

        // The MSR protocoll wants a bufsize, the maximum number of
        // values that can be retraced. This artificial limitation does
        // not exist any more. Instead, choose a buffer size so that
        // at a maximum of 10 seconds has to be stored.
        size_t bufsize = 10 * std::max( 1U, (unsigned int)(freq + 0.5));

        if (shortReply)
            return;

        // bufsize=
        XmlElement::Attribute(element, "bufsize") << bufsize;
        XmlElement::Attribute(element, "HZ") << freq;

        if (data)
            XmlElement::Attribute(element, "value").csv(this,
                    data + offset, 1, precision);
    }

    addCompoundFields(element, dtype);
}
