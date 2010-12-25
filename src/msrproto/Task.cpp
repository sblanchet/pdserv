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
#include "Channel.h"
#include "SubscriptionManager.h"
#include "Subscription.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Task::Task(SubscriptionManager& s): subscriptionManager(s)
{
}

/////////////////////////////////////////////////////////////////////////////
Task::~Task()
{
    delChannel();
}

/////////////////////////////////////////////////////////////////////////////
void Task::delChannel(const Channel *c)
{
    if (!c) {
        ChannelSubscriptionMap::const_iterator it;
        for (it = channelSubscriptionMap.begin();
                it != channelSubscriptionMap.end(); it++)
            delete it->second;

        activeSet.clear();
        channelSubscriptionMap.clear();
    }
    else {
        ChannelSubscriptionMap::iterator it =
            channelSubscriptionMap.find(c);

        if (it == channelSubscriptionMap.end())
            return;

        activeSet[c->signal].erase(it->second);
        delete it->second;
        channelSubscriptionMap.erase(it);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Task::addChannel(const Channel *c,
        bool event, bool sync, unsigned int decimation, size_t blocksize,
        bool base64, size_t precision)
{
//    cout << __PRETTY_FUNCTION__ << signal->path << endl;
    ChannelSubscriptionMap::iterator it = channelSubscriptionMap.find(c);
    Subscription *subscription = channelSubscriptionMap[c];
    if (!subscription) {
        subscription = new Subscription(c, subscriptionManager);
        channelSubscriptionMap[c] = subscription;
        activeSet[c->signal].insert(subscription);
    }

    subscription->set(event, sync, decimation, blocksize, base64, precision);
}

/////////////////////////////////////////////////////////////////////////////
bool Task::newSignalList(const HRTLab::Signal * const *s, size_t n)
{
    bool sync = false;

    // Since it is not (should not be!) possible that required signal 
    // is not transmitted any more, only need to check for new signals
    for (; n--; s++) {
        ActiveSet::iterator it = activeSet.find(*s);
        if (it == activeSet.end())
            continue;

        for (SubscriptionSet::const_iterator sd = it->second.begin();
                sd != it->second.end(); sd++)
            if ((*sd)->activate())
                sync = true;
    }

    return sync;
}

/////////////////////////////////////////////////////////////////////////////
void Task::newSignalValues(MsrXml::Element *parent,
        const HRTLab::Receiver &receiver)
{
    for (ActiveSet::iterator it = activeSet.begin();
            it != activeSet.end(); it++) {

        for (SubscriptionSet::const_iterator sd = it->second.begin();
                sd != it->second.end(); sd++)
            (*sd)->newValue(parent, receiver);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Task::sync()
{
    for (ActiveSet::iterator it = activeSet.begin();
            it != activeSet.end(); it++) {

        for (SubscriptionSet::const_iterator sd = it->second.begin();
                sd != it->second.end(); sd++)
            (*sd)->sync();
    }
}
