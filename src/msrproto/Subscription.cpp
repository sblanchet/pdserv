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
#include "../DataType.h"

#include <algorithm>

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Subscription::Subscription(const Channel *channel, bool event,
        unsigned int decimation, size_t blocksize,
        bool base64, size_t precision):
    channel(channel),
    event(event), decimation(event ? 1 : decimation),
    bufferOffset(channel->offset)
{
    trigger = 0;
    nblocks = 0;

    if (!decimation)
        decimation = 1U;

    if (!blocksize)
        blocksize = 1U;

    if (event) {
        this->trigger_start = decimation;
        this->trigger = 0;
        this->blocksize = 1;
    }
    else {
        this->blocksize = blocksize;
    }
    this->precision = precision;
    this->base64 = base64;

    size_t dataLen = this->blocksize * channel->memSize;

    data_bptr = new char[dataLen];
    data_eptr = data_bptr + dataLen;

    data_pptr = data_bptr;
}

/////////////////////////////////////////////////////////////////////////////
Subscription::~Subscription()
{
    delete data_bptr;
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
//    if (!nblocks)
//        return;

    XmlElement datum(event ? "E" : "F", parent);
    XmlElement::Attribute(datum, "c") << channel->index;

    XmlElement::Attribute value(datum, "d");
    if (base64)
        value.base64(data_bptr, nblocks * channel->memSize);
    else
        value.csv(channel, data_bptr, nblocks, precision);

    data_pptr = data_bptr;
    nblocks = 0;
}
