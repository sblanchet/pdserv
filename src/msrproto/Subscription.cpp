/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdserv package.
 *
 *  pdserv is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pdserv is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pdserv. See COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
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
    bufferOffset(channel->elementIndex * channel->signal->width)
{
    data_bptr = 0;
    trigger = 0;
}

/////////////////////////////////////////////////////////////////////////////
Subscription::~Subscription()
{
}

/////////////////////////////////////////////////////////////////////////////
void Subscription::set( bool event, bool sync, unsigned int decimation,
        size_t blocksize, bool base64, size_t precision)
{
//    cout << __PRETTY_FUNCTION__ << signal->path << endl;
    this->event = event;
    this->sync = sync;
    this->precision = precision;
    this->base64 = base64;

    this->decimation = std::max(1U, decimation);
    this->blocksize = event ? 1 : std::max(static_cast<size_t>(1), blocksize);

    this->trigger %= this->decimation;

    size_t dataLen = this->blocksize * channel->memSize;
    if (data_eptr != data_bptr + dataLen) {
        delete[] data_bptr;
        data_bptr = new char[dataLen];
        data_eptr = data_bptr + dataLen;

        data_pptr = data_bptr;
        this->trigger = 1;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Subscription::newValue( PrintQ &printQ, const void *dataBuf)
{
    const char *buf = reinterpret_cast<const char *>(dataBuf) + bufferOffset;
    if (trigger and --trigger)
        return;

    nblocks = 0;
    if (!event or
            !std::equal(static_cast<const char*>(data_bptr), data_eptr, buf)) {
        trigger = decimation;

        std::copy(buf, buf + channel->memSize, data_pptr);

        if (event) {
            nblocks = 1;
        }
        else {
            data_pptr += channel->memSize;
            if (data_pptr == data_eptr) {
                data_pptr = data_bptr;
                nblocks = blocksize;
            }
        }
    }

    if (nblocks)
        printQ.push(this);
}

/////////////////////////////////////////////////////////////////////////////
void Subscription::print(XmlElement &parent) const
{
    XmlElement datum("F", parent);
    XmlElement::Attribute(datum, "c") << channel->variableIndex;

    XmlElement::Attribute value(datum, "d");
    if (base64)
        value.base64(data_bptr, nblocks * channel->memSize);
    else
        value.csv(channel, data_bptr, nblocks, precision);
}

/////////////////////////////////////////////////////////////////////////////
bool Subscription::reset()
{
    bool sync = this->sync;

    this->sync = false;
    data_pptr = data_bptr;
    trigger = decimation;

    return sync;
}
