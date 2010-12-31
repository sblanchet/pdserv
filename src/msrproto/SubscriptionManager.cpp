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

#include "../Main.h"
#include "../Signal.h"
#include "../Receiver.h"
#include "SubscriptionManager.h"
#include "Channel.h"
#include "Session.h"
#include "Subscription.h"

#include <algorithm>

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
SubscriptionManager::~SubscriptionManager()
{
}

/////////////////////////////////////////////////////////////////////////////
bool SubscriptionManager::subscribe(const Channel *c,
        bool event, bool sync, unsigned int decimation,
        size_t blocksize, bool base64, size_t precision)
{
    bool empty = signalSubscriptionMap[c->signal].empty();
    //cout << __LINE__ << __PRETTY_FUNCTION__ << empty << endl;

    Subscription *subscription = signalSubscriptionMap[c->signal][c];
    if (!subscription) {
        subscription = new Subscription(c);
        signalSubscriptionMap[c->signal][c] = subscription;
    }
    subscription->set(event, sync, decimation, blocksize, base64, precision);

    return empty;
}

/////////////////////////////////////////////////////////////////////////////
bool SubscriptionManager::unsubscribe(const Channel *c)
{
    if (!c) {
        signalSubscriptionMap.clear();
        return true;
    }
//    cout << __LINE__ << __PRETTY_FUNCTION__ << c->path() << endl;

    signalSubscriptionMap[c->signal].erase(c);
    if (signalSubscriptionMap[c->signal].empty()) {
        signalSubscriptionMap.erase(c->signal);
        activeSignalSet.erase(c->signal);
        return true;
    }
    return false;
}

/////////////////////////////////////////////////////////////////////////////
bool SubscriptionManager::newSignalList(
        const HRTLab::Signal * const *s, size_t n)
{
//    cout << __func__ << n << endl;

    bool sync = false;
//    cout << __func__ << __LINE__ << signalSubscriptionMap.size() << endl;

    for (; n--; s++) {
        SignalSubscriptionMap::iterator sit =
            signalSubscriptionMap.find(*s);

//        cout << __LINE__ << (*s)->path << bufOffset << endl;
        if (sit == signalSubscriptionMap.end())
            continue;

        if (sit->second.sync())
            sync = true;

        activeSignalSet.insert(*s);
    }

    return sync;
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::newSignalData(MsrXml::Element *parent,
        const HRTLab::Receiver& receiver)
{
//    cout << __func__ << receiver.seqNo << endl;
    for (ActiveSignalSet::const_iterator sit = activeSignalSet.begin();
            sit != activeSignalSet.end(); sit++) {
        signalSubscriptionMap[*sit].newSignalData(
                parent, receiver.getValue(*sit));
    }
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::sync()
{
//    cout << __func__ << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" << endl;
    for (ActiveSignalSet::const_iterator sit = activeSignalSet.begin();
            sit != activeSignalSet.end(); sit++) {
        signalSubscriptionMap[*sit].sync();
    }
}

/////////////////////////////////////////////////////////////////////////////
SubscriptionManager::SignalSubscription::~SignalSubscription()
{
    for (const_iterator it = begin(); it != end(); it++)
        delete it->second;
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::SignalSubscription::newSignalData(
        MsrXml::Element *parent, const char *data)
{
    for (const_iterator it = begin(); it != end(); it++) {
        it->second->newValue(parent, data);
    }
}

/////////////////////////////////////////////////////////////////////////////
bool SubscriptionManager::SignalSubscription::sync()
{
    bool sync = false;
    for (const_iterator it = begin(); it != end(); it++) {
        if (it->second->sync())
            sync = true;
    }
    return sync;
}
