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
#include "SubscriptionChange.h"

#include <algorithm>

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
SubscriptionChange::SubscriptionChange(HRTLab::Main *main,
        const HRTLab::Session *session):
    main(main), session(session)
{
    state = 0;
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionChange::subscribe(const HRTLab::Signal *s)
{
//    cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    unsubscribeSet.erase(s);
    subscribeSet.insert(s);

    state = 2;
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionChange::unsubscribe(const HRTLab::Signal *s)
{
//    cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    unsubscribeSet.insert(s);
    subscribeSet.erase(s);

    state = 2;
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionChange::process()
{
//    cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    if (!state or --state)
        return;

//    cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    const HRTLab::Signal *ins[subscribeSet.size()];
    std::copy(subscribeSet.begin(), subscribeSet.end(), ins);

//    cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    const HRTLab::Signal *del[unsubscribeSet.size()];
    std::copy(unsubscribeSet.begin(), unsubscribeSet.end(), del);

//    cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    main->unsubscribe(session, del, unsubscribeSet.size());
//    cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    main->subscribe(session, ins, subscribeSet.size());
//    cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
}

