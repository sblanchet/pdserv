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
    SessionTask(task, taskStatistics, taskTime), session(s)
{
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::subscribe (const Channel *c,
        bool event, unsigned int decimation,
        size_t blocksize, bool base64, size_t precision)
{
    Subscription *subscription = signalSubscriptionMap[c->signal].find(c);
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

    if (it->second.erase(c)) {
        signalSubscriptionMap.erase(it);
        activeSignalSet.erase(&it->second);
        c->signal->unsubscribe(this);
    }
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::newSignal( const PdServ::Signal *s)
{
    SignalSubscriptionMap::iterator sit = signalSubscriptionMap.find(s);

    // Find out whether this signal is used or whether it is active already
    if (sit != signalSubscriptionMap.end()) {
        activeSignalSet.insert(&sit->second);
        log_debug("%s", s->path.c_str());
    }
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::rxPdo(std::ostream& os,
        ost::Semaphore& streamlock, bool quiet)
{
    typedef std::queue<Subscription*> PrintQ;
    PrintQ printQ;
    ActiveSet::const_iterator it;
    ActiveSet::const_iterator it_end;
    ChannelSubscription::const_iterator it2, it2_end;

    while (task->rxPdo(this, &taskTime, &taskStatistics)) {
        it_end = activeSignalSet.end();
        for (it = activeSignalSet.begin(); it != it_end; it++) {
            it2 = (*it)->begin();
            it2_end = (*it)->end();

            const char *data = (*it2)->channel->signal->getValue(this);

            do {
                if ((*it2)->newValue(data))
                    printQ.push(*it2);
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
    for (ActiveSet::iterator it = activeSignalSet.begin();
            it != activeSignalSet.end(); it++)
        (*it)->sync();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
SubscriptionManager::ChannelSubscription::~ChannelSubscription()
{
    for (const_iterator it = begin(); it != end(); it++) {
        //cout << __func__ << it->second << endl;
        delete *it;
    }
}

/////////////////////////////////////////////////////////////////////////////
Subscription* SubscriptionManager::ChannelSubscription::find (const Channel* c)
{
    ChannelSubscriptionMap::const_iterator it = map.find(c);
    if (it == map.end()) {
        Subscription* subscription = new Subscription(c);
        map[c] = size();
        push_back(subscription);
        log_debug("Subscription not available. Creating...");
        return subscription;
    }
    else
        return operator[](it->second);
}

/////////////////////////////////////////////////////////////////////////////
bool SubscriptionManager::ChannelSubscription::erase (const Channel* c)
{
    ChannelSubscriptionMap::iterator it = map.find(c);
    if (it != map.end()) {
        SubscriptionVector::operator[](it->second) = back();
        map[back()->channel] = it->second;
        SubscriptionVector::pop_back();
        map.erase(it);
    }

    return empty();
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::ChannelSubscription::sync ()
{
    for (const_iterator it = begin(); it != end(); it++)
        (*it)->reset();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::ActiveSet::erase (const ChannelSubscription* s)
{
    ChannelSubscriptionMap::iterator it = map.find(s);
    if (it == map.end())
        return;

    ChannelSubscriptionVector::operator[](it->second) = back();
    map[back()] = it->second;
    ChannelSubscriptionVector::pop_back();
    map.erase(it);
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::ActiveSet::insert (ChannelSubscription* s)
{
    ChannelSubscriptionMap::iterator it = map.find(s);
    if (it != map.end())
        return;

    map[s] = size();
    push_back(s);
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::ActiveSet::clear ()
{
    map.clear();
    ChannelSubscriptionVector::clear();
}
