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
#include "../TaskStatistics.h"
#include "SubscriptionManager.h"
#include "Channel.h"
#include "Session.h"
#include "Subscription.h"
#include "XmlElement.h"

#include <algorithm>
#include <queue>

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
struct timespec SubscriptionManager::dummyTime;
PdServ::TaskStatistics SubscriptionManager::dummyTaskStatistics;

/////////////////////////////////////////////////////////////////////////////
SubscriptionManager::SubscriptionManager(
        const Session *s, const PdServ::Task* task):
    SessionTask(task), session(s)
{
    taskTime = &dummyTime;
    taskStatistics = &dummyTaskStatistics;

    unsubscribedCount = 0;
    subscribedCount = 0;
    doSync = false;
}

/////////////////////////////////////////////////////////////////////////////
SubscriptionManager::~SubscriptionManager()
{
    clear();
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::subscribe (const Channel *c,
        bool event, unsigned int decimation,
        size_t blocksize, bool base64, size_t precision)
{
    remove(signalSubscriptionMap[c->signal][c]);

    Subscription *subscription =
        new Subscription(c, event, decimation, blocksize, base64, precision);
    signalSubscriptionMap[c->signal][c] = subscription;

    DecimationTuple& dt = activeSignals[subscription->decimation];
    if (!dt.first)
        dt.first = 1;

    ++unsubscribedCount;
    c->signal->subscribe(this);
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::clear()
{
    SignalSubscriptionMap::const_iterator it;
    for (it = signalSubscriptionMap.begin();
            it != signalSubscriptionMap.end(); ++it) {
        it->first->unsubscribe(this);
        for (ChannelSubscriptionMap::const_iterator it2 = it->second.begin();
                it2 != it->second.end(); ++it2)
            delete it2->second;
    }

    signalSubscriptionMap.clear();
    activeSignals.clear();

    subscribedCount = 0;
    unsubscribedCount = 0;
    doSync = 0;
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::remove (Subscription *s)
{
    if (!s)
        return;

    DecimationGroup::iterator it = activeSignals.find(s->decimation);
    if (it != activeSignals.end()) {
        SubscriptionSet::iterator it2 = it->second.second.find(s);
        if (it2 != it->second.second.end()) {
            it->second.second.erase(s);
            if (it->second.second.empty())
                activeSignals.erase(it);
            --subscribedCount;
        }
        else
            --unsubscribedCount;
    }

    delete s;
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::unsubscribe(const Channel *c)
{
//    cout << __func__ << c->path() << endl;
    SignalSubscriptionMap::iterator it = signalSubscriptionMap.find(c->signal);

    if (it == signalSubscriptionMap.end())
        return;

    ChannelSubscriptionMap::iterator it2 = it->second.find(c);
    if (it2 == it->second.end())
        return;

    remove(it2->second);

    it->second.erase(it2);
    if (it->second.empty()) {
        it->first->unsubscribe(this);
        signalSubscriptionMap.erase(it);
    }
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::newSignal( const PdServ::Signal *s)
{
    if (!unsubscribedCount)
        return;

    SignalSubscriptionMap::const_iterator sit = signalSubscriptionMap.find(s);

    // Find out whether this signal is used or whether it is active already
    if (sit == signalSubscriptionMap.end())
        return;

    for (ChannelSubscriptionMap::const_iterator it2 = sit->second.begin();
            it2 != sit->second.end(); ++it2) {
        Subscription *s = it2->second;
        DecimationTuple& dt = activeSignals[s->decimation];

        SubscriptionSet::iterator it3 = dt.second.find(s);
        if (it3 == dt.second.end()) {
            dt.second.insert(s);
            doSync = !--unsubscribedCount;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::rxPdo(Session::TCPStream& tcp, bool quiet)
{
    typedef std::queue<Subscription*> PrintQ;
    PrintQ printQ;
    DecimationGroup::iterator it;
    SubscriptionSet::const_iterator it2, it2_end;

    while (task->rxPdo(this, &taskTime, &taskStatistics)) {
        if (quiet)
            continue;

        for (it = activeSignals.begin(); it != activeSignals.end(); ++it) {
            DecimationTuple& dt = it->second;

            if (--dt.first)
                continue;

            dt.first = it->first;
            it2_end = dt.second.end();

            for (it2 = dt.second.begin(); it2 != it2_end; ++it2) {
                const char *data = (*it2)->channel->signal->getValue(this);
                if ((*it2)->newValue(data) or (doSync and !(*it2)->event))
                    printQ.push(*it2);
            }
        }
        doSync = false;

        if (!printQ.empty()) {
            XmlElement dataTag(tcp.createElement("data"));
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
    doSync = !unsubscribedCount;
    for (DecimationGroup::iterator it = activeSignals.begin();
            it != activeSignals.end(); ++it)
        it->second.first = 1;
}
