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

#include "../Signal.h"
#include "../Debug.h"
#include "XmlElement.h"
#include "Channel.h"
#include "SubscriptionManager.h"
#include "Subscription.h"
#include "Session.h"

#include <algorithm>
#include <cstdio> // snprintf

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Subscription::Subscription(const Channel* channel):
    channel(channel),
    bufferOffset(channel->elementIndex * channel->signal->elemSize)
{
    data_bptr = 0;
    data_eptr = 0;
    trigger = 0;
    nblocks = 0;
}

/////////////////////////////////////////////////////////////////////////////
Subscription::~Subscription()
{
}

/////////////////////////////////////////////////////////////////////////////
void Subscription::set( bool event, unsigned int decimation,
        size_t blocksize, bool base64, size_t precision)
{
//    cout << __PRETTY_FUNCTION__ << signal->path << endl;
    if (!decimation)
        decimation = 1U;

    if (!blocksize)
        blocksize = 1U;

    if (event) {
        this->event = 1;
        this->decimation = 1;
        this->trigger_start = decimation;
        this->trigger = 0;
        this->blocksize = 1;
    }
    else {
        this->event = 0;
        this->decimation = decimation;
        this->blocksize = blocksize;
    }
    this->precision = precision;
    this->base64 = base64;

    size_t dataLen = this->blocksize * channel->memSize;
    if (data_eptr < data_bptr + dataLen) {
        data_bptr = new char[dataLen];
        data_eptr = data_bptr + dataLen;

        data_pptr = data_bptr;
    }
}

/////////////////////////////////////////////////////////////////////////////
bool Subscription::newValue (const char *buf)
{
    const size_t n = channel->memSize;
    buf += bufferOffset;
    if (event) {
        if (!std::equal(buf, buf + n, data_bptr)) {
            std::copy(buf, buf + n, data_bptr);
            nblocks = 1;
            data_pptr = data_bptr + n;
        }

        if (!trigger) {
            if (nblocks)
                trigger = trigger_start;
            return nblocks;
        }
        --trigger;
        return false;
    }

    std::copy(buf, buf + n, data_pptr);
    data_pptr += n;
    ++nblocks;

    return nblocks == blocksize;
}

/////////////////////////////////////////////////////////////////////////////
void Subscription::print(XmlElement &parent)
{
    XmlElement datum(event ? "E" : "F", parent);
    XmlElement::Attribute(datum, "c") << channel->variableIndex;

    XmlElement::Attribute value(datum, "d");
    if (base64)
        value.base64(data_bptr, nblocks * channel->memSize);
    else
        value.csv(channel, data_bptr, nblocks, precision);

    data_pptr = data_bptr;
    nblocks = 0;
}
