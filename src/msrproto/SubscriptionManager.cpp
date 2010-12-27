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
SubscriptionManager::SubscriptionManager(Session *session):
    session(session)
{
}

/////////////////////////////////////////////////////////////////////////////
SubscriptionManager::~SubscriptionManager()
{
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::subscribe(const Channel *c,
        bool event, bool sync, unsigned int decimation,
        size_t blocksize, bool base64, size_t precision)
{
//    cout << __LINE__ << __PRETTY_FUNCTION__ << c->path() << c->signal->task << endl;
    size_t offset = ~0U;

    SignalSubscriptionMap& signalSubscriptionMap =
        taskSubscription[c->signal->task];
    ChannelSubscriptionMap& csMap = signalSubscriptionMap[c->signal];

    if (csMap.empty()) {
        // This signal is not yet transferred from the real time process;
        // get it
        session->requestSignal(c->signal, true);
    }
    else {
        offset = csMap.begin()->second->getOffset();
    }

    Subscription *subscription = csMap[c];
    if (!subscription) {
        subscription = new Subscription(c);
        csMap[c] = subscription;
    }

    subscription->set(event, sync, decimation, blocksize, base64, precision);

    if (offset != ~0U and subscription->activate(offset))
        this->sync();
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::unsubscribe(const Channel *c)
{
    if (!c) {
        for (TaskSubscription::const_iterator it = taskSubscription.begin();
                it != taskSubscription.end(); it++) {
            while (!it->second.empty()) {
                SignalSubscriptionMap::const_iterator sit = it->second.begin();
                while (!sit->second.empty()) {
                    unsubscribe(sit->second.begin()->first);
                }
            }
        }
        return;
    }
//    cout << __LINE__ << __PRETTY_FUNCTION__ << c->path() << endl;

    SignalSubscriptionMap& signalSubscriptionMap =
        taskSubscription[c->signal->task];

    SignalSubscriptionMap::iterator sit =
        signalSubscriptionMap.find(c->signal);

    if (sit == signalSubscriptionMap.end())
        return;

    ChannelSubscriptionMap::iterator cit = sit->second.find(c);

    if (cit == sit->second.end())
        return;

    delete cit->second;

    sit->second.erase(cit);
    if (sit->second.empty()) {
        signalSubscriptionMap.erase(sit);
        session->requestSignal(c->signal, false);
    }
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::newSignalList(const HRTLab::Task *task,
        const HRTLab::Signal * const *s, size_t n)
{
//    cout << __func__ << n << endl;

    bool sync = false;
    size_t bufOffset = 0;
    SignalSubscriptionMap& signalSubscriptionMap = taskSubscription[task];

//    cout << __func__ << __LINE__ << signalSubscriptionMap.size() << endl;
    for (; n--; s++) {
        SignalSubscriptionMap::const_iterator sit =
            signalSubscriptionMap.find(*s);

//        cout << __LINE__ << (*s)->path << bufOffset << endl;
        if (sit != signalSubscriptionMap.end()) {
            ChannelSubscriptionMap::const_iterator cit;
            for (cit = sit->second.begin(); cit != sit->second.end(); cit++) {
                Subscription *s = cit->second;

//                cout << __func__ << __LINE__ << endl;
                if (s->activate(bufOffset))
                    sync = true;
            }
        }
        bufOffset += (*s)->memSize;
    }

    if (sync)
        this->sync();
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::newSignalData(MsrXml::Element *parent,
        const HRTLab::Receiver& receiver, const char *data)
{
//    cout << __func__ << receiver.seqNo << endl;
    SignalSubscriptionMap& signalSubscriptionMap =
        taskSubscription[receiver.task];

    for (SignalSubscriptionMap::const_iterator sit = signalSubscriptionMap.begin();
            sit != signalSubscriptionMap.end(); sit++) {
        for (ChannelSubscriptionMap::const_iterator cit = sit->second.begin();
                cit != sit->second.end(); cit++) {
            cit->second->newValue(parent, data);
        }
    }

}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::sync()
{
//    cout << __func__ << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" << endl;
    for (TaskSubscription::const_iterator it = taskSubscription.begin();
            it != taskSubscription.end(); it++) {
        for (SignalSubscriptionMap::const_iterator sit = it->second.begin();
                sit != it->second.end(); sit++) {
            for (ChannelSubscriptionMap::const_iterator cit = sit->second.begin();
                    cit != sit->second.end(); cit++)
                cit->second->sync();
        }
    }
}
