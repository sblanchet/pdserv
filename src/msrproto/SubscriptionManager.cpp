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

#include "../Main.h"
#include "../Signal.h"
#include "../SessionTaskData.h"
#include "SubscriptionManager.h"
#include "Channel.h"
#include "Session.h"
#include "Subscription.h"

#include <algorithm>

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
SubscriptionManager::SubscriptionManager(Session *s):
    session(s)
{
}

/////////////////////////////////////////////////////////////////////////////
SubscriptionManager::~SubscriptionManager()
{
    clear();
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::subscribe(const Channel *c,
        bool event, bool sync, unsigned int decimation,
        size_t blocksize, bool base64, size_t precision)
{
//    cout << __func__ << c->path() << endl;
    Subscription *subscription = signalSubscriptionMap[c->signal][c];
    if (!subscription) {
        subscription = new Subscription(c);
        signalSubscriptionMap[c->signal][c] = subscription;
        c->signal->subscribe(session);
    }
    subscription->set(event, sync, decimation, blocksize, base64, precision);
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::clear()
{

    SignalSubscriptionMap::const_iterator it = signalSubscriptionMap.begin();
    while (it != signalSubscriptionMap.end())
        (it++)->first->unsubscribe(session);

    signalSubscriptionMap.clear();
    activeSignalSet.clear();
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::unsubscribe(const Channel *c)
{
//    cout << __func__ << c->path() << endl;
    delete signalSubscriptionMap[c->signal][c];

    signalSubscriptionMap[c->signal].erase(c);
    if (signalSubscriptionMap[c->signal].empty()) {
        signalSubscriptionMap.erase(c->signal);
        activeSignalSet.erase(c->signal);
        c->signal->unsubscribe(session);
    }
}

/////////////////////////////////////////////////////////////////////////////
bool SubscriptionManager::newSignalList(
        const PdServ::Signal * const *s, size_t n)
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
void SubscriptionManager::newSignalData(
        PrintQ &printQ, const PdServ::SessionTaskData *std)
{
//    cout << __func__ << receiver.seqNo << endl;
    for (ActiveSignalSet::const_iterator sit = activeSignalSet.begin();
            sit != activeSignalSet.end(); sit++) {
        signalSubscriptionMap[*sit].newSignalData(
                printQ, (*sit)->getValue(std));
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
    for (const_iterator it = begin(); it != end(); it++) {
        //cout << __func__ << it->second << endl;
        delete it->second;
    }
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::SignalSubscription::newSignalData(
        PrintQ &printQ, const void *data)
{
    for (const_iterator it = begin(); data and it != end(); it++) {
        it->second->newValue(printQ, data);
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
