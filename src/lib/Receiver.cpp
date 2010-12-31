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
Receiver::Receiver(Task *task, /*unsigned int tid,*/ TxFrame *start):
    HRTLab::Receiver(task), task(task), mutex(1), rxPtr(start)
{
    currentListId = ~0U;
     while (rxPtr->next)
         rxPtr = rxPtr->next;
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
//             cout << __LINE__ << "looking for " << rxPtr->signalListNo << endl;
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
//    cout << __LINE__ << __func__ << ' ' << listId << ' ' << n << endl;
    ost::SemaphoreLock lock(mutex);
    const HRTLab::Signal **signals = new const HRTLab::Signal*[n+1];

    std::copy(s, s+n, signals);
    signals[n] = 0;

//    for (size_t i = 0; i < n; i++)
//        cout << s[i]->path << endl;

    listIdQ.push(ListIdPair(listId, signals));
}
