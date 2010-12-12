/*****************************************************************************
 * $Id$
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
    unsubscribeSet.erase(s);
    subscribeSet.insert(s);

    state = 2;
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionChange::unsubscribe(const HRTLab::Signal *s)
{
    unsubscribeSet.insert(s);
    subscribeSet.erase(s);

    state = 2;
}

/////////////////////////////////////////////////////////////////////////////
void SubscriptionChange::process()
{
    if (!state or --state)
        return;

    const HRTLab::Signal *ins[subscribeSet.size()];
    std::copy(subscribeSet.begin(), subscribeSet.end(), ins);

    const HRTLab::Signal *del[unsubscribeSet.size()];
    std::copy(unsubscribeSet.begin(), unsubscribeSet.end(), del);

    main->unsubscribe(session, del, unsubscribeSet.size());
    main->subscribe(session, ins, subscribeSet.size());
}

