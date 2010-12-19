/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdcomserv package.
 *
 *  pdcomserv is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pdcomserv is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pdcomserv. See COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include "config.h"

#include "../Signal.h"
#include "../Receiver.h"

#include "XmlDoc.h"
#include "Task.h"
#include "SubscriptionChange.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Task::Task(SubscriptionChange& s): subscriptionChange(s)
{
}

/////////////////////////////////////////////////////////////////////////////
Task::~Task()
{
    for (SubscribedSet::const_iterator it = subscribedSet.begin();
            it != subscribedSet.end(); it++) {
        delete[] it->second.data_bptr;
        delete it->second.element;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Task::delSignal(const HRTLab::Signal *signal)
{
    if (!signal) {
        while (!subscribedSet.empty())
            delSignal(subscribedSet.begin()->first);

        return;
    }

    SubscribedSet::iterator it = subscribedSet.find(signal);
    if (it == subscribedSet.end())
        return;

    delete[] it->second.data_bptr;
    delete it->second.element;
    subscribedSet.erase(it);
    activeSet.erase(signal);

    subscriptionChange.unsubscribe(signal);
}

/////////////////////////////////////////////////////////////////////////////
void Task::addSignal(const HRTLab::Signal *signal, unsigned int idx,
        bool event, bool sync, unsigned int decimation, size_t blocksize,
        bool base64, size_t precision)
{
//    cout << __PRETTY_FUNCTION__ << signal->path << endl;
    SubscribedSet::iterator it = subscribedSet.find(signal);
    if (it == subscribedSet.end()) {
        subscriptionChange.subscribe(signal);
    }
    else {
        delete[] it->second.data_bptr;
        delete it->second.element;
    }

    if (event)
        // Event triggering
        blocksize = 1;
    else if (!blocksize)
        blocksize = 1;

    size_t dataLen = blocksize * signal->memSize;
    char *data = new char[dataLen];
    SignalData sd = {
        signal,
        new MsrXml::Element("F"),
        event,
        sync,
        decimation,
        0,      // trigger
        blocksize,
        signal->memSize,
        (base64
         ? &MsrXml::Element::base64ValueAttr
         : &MsrXml::Element::csvValueAttr),
        precision,
        data,
        data,
        data + dataLen
    };

    sd.element->setAttribute("c", idx);

    subscribedSet[signal] = sd;
}

/////////////////////////////////////////////////////////////////////////////
bool Task::newSignalList(const HRTLab::Signal * const *s, size_t n)
{
    bool sync = false;

    // Since it is not (should not be!) possible that required signal 
    // is not transmitted any more, only need to check for new signals
    for (; n--; s++) {
        SubscribedSet::iterator it = subscribedSet.find(*s);
        if (it == subscribedSet.end())
            continue;

        activeSet[*s] = &(it->second);
        if (it->second.sync) {
            sync = true;
            it->second.sync = false;
        }
    }


    return sync;
}

/////////////////////////////////////////////////////////////////////////////
void Task::newSignalValues(MsrXml::Element *parent,
        const HRTLab::Receiver &receiver)
{
    for (ActiveSet::iterator it = activeSet.begin();
            it != activeSet.end(); it++) {

        SignalData *sd = it->second;

        if (sd->trigger and --sd->trigger)
            continue;

        const char* dataBuf = receiver.getValue(sd->signal);

        if (!sd->event) {
            sd->trigger = sd->decimation;

            std::copy(dataBuf, dataBuf + sd->sigMemSize, sd->data_pptr);
            sd->data_pptr += sd->sigMemSize;
            if (sd->data_pptr == sd->data_eptr) {
                (sd->element->*(sd->printValue))("d", sd->signal,
                        sd->data_bptr, sd->precision, sd->blocksize);
                parent->appendChild(sd->element);
                sd->data_pptr = sd->data_bptr;
            }
        }
        else if (!std::equal(sd->data_bptr, sd->data_eptr, dataBuf)) {
            sd->trigger = sd->decimation;

            std::copy(dataBuf, dataBuf + sd->sigMemSize, sd->data_bptr);
            (sd->element->*(sd->printValue))("d", sd->signal,
                    sd->data_bptr, sd->precision, 1);
            parent->appendChild(sd->element);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
void Task::sync()
{
    for (ActiveSet::iterator it = activeSet.begin();
            it != activeSet.end(); it++) {
        SignalData *sd = it->second;

        sd->trigger = sd->decimation;
        sd->data_pptr = sd->data_bptr;
    }
}

