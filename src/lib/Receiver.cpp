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

#include <algorithm>

#include "Receiver.h"
#include "Signal.h"
#include "Task.h"
#include "../Session.h"
#include "../Signal.h"

////////////////////////////////////////////////////////////////////////////
Receiver::Receiver(Task *task, unsigned int tid, TxFrame *start):
    HRTLab::Receiver(tid), task(task), mutex(1), rxPtr(start)
{
    currentListId = ~0U;
}

////////////////////////////////////////////////////////////////////////////
Receiver::~Receiver()
{
    task->receiverDeleted(this);

    while (!listIdQ.empty()) {
        delete[] listIdQ.front().second;
        listIdQ.pop();
    }
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
         //cout << __LINE__ << __PRETTY_FUNCTION__ << rxPtr->seqNo << endl;
         if (currentListId != rxPtr->signalListNo) {
             //cout << __LINE__ << "looking for " << rxPtr->signalListNo << endl;
             ost::SemaphoreLock lock(mutex);

             currentListId = rxPtr->signalListNo;
             while (!listIdQ.empty()
                     and listIdQ.front().first != currentListId) {
                 delete[] listIdQ.front().second;
                 listIdQ.pop();
             }

             if (listIdQ.empty())
                 return;

             size_t count;
             size_t _offset = 0;
             const HRTLab::Signal *const *s = listIdQ.front().second;
             for (count = 0; s[count]; count++) {
                 offset[s[count]] = _offset;
                 _offset += s[count]->memSize;
             }
             session->newSignalList(task, s, count);

             delete[] listIdQ.front().second;
             listIdQ.pop();
         }

         seqNo = rxPtr->seqNo;
         time = &rxPtr->time;
         session->newSignalData(*this);
         rxPtr = rxPtr->next;
     }
}

////////////////////////////////////////////////////////////////////////////
void Receiver::newSignalList(unsigned int listId,
        const HRTLab::Signal * const *s, size_t n)
{
    //cout << __LINE__ << __func__ << ' ' << n << endl;
    ost::SemaphoreLock lock(mutex);
    const HRTLab::Signal **signals = new const HRTLab::Signal*[n+1];

    std::copy(s, s+n, signals);
    signals[n] = 0;

    listIdQ.push(ListIdPair(listId, signals));
}
