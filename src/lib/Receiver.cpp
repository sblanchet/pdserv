/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "config.h"
#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

#include "Receiver.h"
#include "Signal.h"
#include "../Session.h"
#include "../Signal.h"

////////////////////////////////////////////////////////////////////////////
Receiver::Receiver(unsigned int tid, TxFrame *start):
    HRTLab::Receiver(tid), rxPtr(start)
{
}

////////////////////////////////////////////////////////////////////////////
Receiver::~Receiver()
{
}

////////////////////////////////////////////////////////////////////////////
const char *Receiver::getValue(const HRTLab::Signal *s) const
{
    return rxPtr->data + offset[s];
}

////////////////////////////////////////////////////////////////////////////
void Receiver::process(HRTLab::Session *session)
{
     while (rxPtr->next) {
         seqNo = rxPtr->seqNo;
         time = &rxPtr->time;
         session->newSignalData(*this);
         rxPtr = rxPtr->next;
     }
}
