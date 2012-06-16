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

#include "../Debug.h"
#include "../Main.h"
#include "../Signal.h"
#include "../Task.h"
#include "SubscriptionManager.h"
#include "Channel.h"
#include "Session.h"
#include "Subscription.h"

#include <algorithm>

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
SubscriptionManager::SubscriptionManager(
        const Session *s, const PdServ::Task* task):
    SessionTask(task), session(s)
{
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::subscribe (const Channel *c,
        bool event, unsigned int decimation,
        size_t blocksize, bool base64, size_t precision)
{
    Subscription *subscription = signalSubscriptionMap[c->signal][c];
    if (!subscription) {
        subscription = new Subscription(c);
        signalSubscriptionMap[c->signal][c] = subscription;
        log_debug("Subscription not available. Creating...");
    }
    log_debug("Setting subscription event");
    subscription->set(event, decimation, blocksize, base64, precision);
    c->signal->subscribe(this);
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::clear()
{
    SignalSubscriptionMap::const_iterator it;
    for (it = signalSubscriptionMap.begin();
            it != signalSubscriptionMap.end(); it++)
        it->first->unsubscribe(this);

    signalSubscriptionMap.clear();
    activeSignalSet.clear();
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::unsubscribe(const Channel *c)
{
//    cout << __func__ << c->path() << endl;
    SignalSubscriptionMap::iterator it =
        signalSubscriptionMap.find(c->signal);

    if (it == signalSubscriptionMap.end())
        return;

    SignalSubscription::iterator it2 = it->second.find(c);

    if (it2 == it->second.end())
        return;

    delete it2->second;

    it->second.erase(it2);

    if (it->second.empty()) {
        signalSubscriptionMap.erase(it);
        activeSignalSet.erase(&it->second);
        c->signal->unsubscribe(this);
    }
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::newSignal( const PdServ::Signal *s)
{
    log_debug("%s", s->path.c_str());
    SignalSubscriptionMap::iterator sit = signalSubscriptionMap.find(s);

    // Find out whether this signal is used or whether it is active already
    if (sit != signalSubscriptionMap.end())
        activeSignalSet.insert(&sit->second);
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::rxPdo(std::ostream& os,
        ost::Semaphore& streamlock, bool quiet)
{
    typedef std::queue<const Subscription*> PrintQ;
    PrintQ printQ;
    ActiveSignalSet::const_iterator it;
    SignalSubscription::const_iterator it2, it2_end;

    while (task->rxPdo(this, &taskTime, &taskStatistics)) {

        for (it = activeSignalSet.begin(); it != activeSignalSet.end(); it++) {
            it2 = (*it)->begin();
            it2_end = (*it)->end();

            const PdServ::Signal *s = it2->first->signal;
            const char *data =
                reinterpret_cast<const char *>(s->getValue(this));

            do {
                if (it2->second->newValue(data))
                    printQ.push(it2->second);
            } while (++it2 != it2_end);
        }

        if (!printQ.empty() and !quiet) {
            XmlElement dataTag("data", os, streamlock);
            XmlElement::Attribute(dataTag, "level") << 0;
            XmlElement::Attribute(dataTag, "time") << *taskTime;

            while (!printQ.empty()) {
                printQ.front()->print(dataTag);
                printQ.pop();
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::sync()
{
    for (ActiveSignalSet::const_iterator sit = activeSignalSet.begin();
            sit != activeSignalSet.end(); sit++) {
        (*sit)->sync();
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
void SubscriptionManager::SignalSubscription::sync()
{
    for (const_iterator it = begin(); it != end(); it++)
        it->second->reset();
}
