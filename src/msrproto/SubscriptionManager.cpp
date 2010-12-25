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
#include "SubscriptionManager.h"
#include "Channel.h"

#include <algorithm>

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
SubscriptionManager::SubscriptionManager(HRTLab::Main *main,
        const HRTLab::Session *session):
    main(main), session(session)
{
    state = 0;
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::subscribe(const Channel *c)
{
//    cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    ChannelSet& cs = channelMap[c->signal];

    if (cs.empty()) {
        pendingUnsubscriptions.erase(c->signal);
        pendingSubscriptions.insert(c->signal);
    }
    cs.insert(c);

    state = 2;
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::unsubscribe(const Channel *c)
{
//    cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    ChannelSet& cs = channelMap[c->signal];

    cs.erase(c);

    if (cs.empty()) {
        pendingUnsubscriptions.insert(c->signal);
        pendingSubscriptions.erase(c->signal);

        channelMap.erase(c->signal);
    }

    state = 2;
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionManager::process()
{
    if (!state or --state)
        return;

    const HRTLab::Signal *ins[pendingSubscriptions.size()];
    std::copy(pendingSubscriptions.begin(), pendingSubscriptions.end(), ins);

    const HRTLab::Signal *del[pendingUnsubscriptions.size()];
    std::copy(pendingUnsubscriptions.begin(), pendingUnsubscriptions.end(), del);

    main->unsubscribe(session, del, pendingUnsubscriptions.size());
    main->subscribe(session, ins, pendingSubscriptions.size());
}

