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

#include <cstdio> // snprintf

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Subscription::Subscription(const Channel* channel):
    channel(channel),
    bufferOffset(channel->elementIndex * channel->signal->width)
{
    data_bptr = 0;
}

/////////////////////////////////////////////////////////////////////////////
Subscription::~Subscription()
{
}

/////////////////////////////////////////////////////////////////////////////
void Subscription::set( bool _event, bool sync, unsigned int _decimation,
        size_t _blocksize, bool _base64, size_t _precision)
{
//    cout << __PRETTY_FUNCTION__ << signal->path << endl;
    event = _event;
    _sync = sync;
    decimation = _decimation ? _decimation-1 : 0;
    trigger = 0;

    if (event)
        // Event triggering
        _blocksize = 1;
    else if (!_blocksize)
        _blocksize = 1;

    blocksize = _blocksize;

    precision = _precision;
    base64 = _base64;

    size_t dataLen = blocksize * channel->memSize;
    if (data_eptr != data_bptr + dataLen) {
        delete[] data_bptr;
        data_bptr = new char[dataLen];
        data_eptr = data_bptr + dataLen;
    }
    data_pptr = data_bptr;
}

/////////////////////////////////////////////////////////////////////////////
void Subscription::newValue( PrintQ &printQ, const void *dataBuf)
{
    const char *buf = reinterpret_cast<const char *>(dataBuf) + bufferOffset;
    if (trigger and trigger--)
        return;

    nblocks = 0;
    if (!event or !std::equal(data_bptr, data_eptr, buf)) {
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
    datum.setAttribute("c", channel->variableIndex);

    if (base64)
        datum.base64Attribute("d", data_bptr, nblocks * channel->memSize);
    else
        datum.csvAttribute("d", channel, data_bptr, nblocks, precision);
}

/////////////////////////////////////////////////////////////////////////////
bool Subscription::sync()
{
    bool sync = _sync;

    _sync = false;
    data_pptr = data_bptr;
    trigger = decimation;

    return sync;
}
