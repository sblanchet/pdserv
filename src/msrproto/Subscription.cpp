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

#include "config.h"

#include "../Signal.h"
#include "../Receiver.h"
//
#include "XmlDoc.h"
#include "PrintVariable.h"
//#include "Task.h"
#include "Channel.h"
#include "SubscriptionManager.h"
#include "Subscription.h"

#include <cstdio> // snprintf

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Subscription::Subscription(const Channel* channel):
    channel(channel), element("F")
{
    element.setAttribute("c", channel->index);
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
void Subscription::newValue(MsrXml::Element *parent, const char *dataBuf)
{
//    cout << __func__ << channel->path() << ' ' << trigger << endl;
    if (trigger and trigger--)
        return;

    dataBuf += channel->bufferOffset;
//    cout << __func__ << channel->path() << offset << ' ' << *(double*)dataBuf << endl;

    if (!event) {
        trigger = decimation;

        std::copy(dataBuf, dataBuf + channel->memSize, data_pptr);
        data_pptr += channel->memSize;
//        cout << ' ' << (data_eptr - data_pptr) / channel->memSize << endl;

        if (data_pptr == data_eptr) {
            if (base64)
                base64Attribute(&element, "d", channel->variable,
                        channel->nelem * blocksize, data_bptr);
            else
                csvAttribute(&element, "d",
                        channel->printFunc, channel->variable,
                        channel->nelem * blocksize, data_bptr);

            parent->appendChild(&element);
            data_pptr = data_bptr;
//            cout << "sent " << channel->path() << endl;
        }
    }
    else if (!std::equal(data_bptr, data_eptr, dataBuf)) {
        trigger = decimation;

        std::copy(dataBuf, dataBuf + channel->memSize, data_bptr);
        if (base64)
            base64Attribute(&element, "d",
                    channel->variable, channel->nelem, data_bptr);
        else
            csvAttribute(&element, "d", channel->printFunc,
                    channel->variable, channel->nelem, data_bptr);
        parent->appendChild(&element);
    }
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
