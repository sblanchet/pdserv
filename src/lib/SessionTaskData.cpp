/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdserv package.
 *
 *  pdserv is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pdserv is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pdserv. See COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include "../Debug.h"
#include "../Session.h"
#include "SessionTaskData.h"
#include "ShmemDataStructures.h"
#include "Task.h"
#include "Signal.h"

////////////////////////////////////////////////////////////////////////////
SessionTaskData::SessionTaskData( PdServ::Session* s, Task* t,
        struct Pdo *txMemBegin, const void *txMemEnd):
    PdServ::SessionTaskData(s, t),
    task(t), session(s), txMemBegin(txMemBegin), txMemEnd(txMemEnd)
{
    signalListId = 0;

    signalPosition.resize(task->signalCount());

    init();
}

////////////////////////////////////////////////////////////////////////////
void SessionTaskData::init()
{
    pdo = txMemBegin;

    do {
        while (!pdo->next)
            ost::Thread::sleep(task->sampleTime * 1000 / 2 + 1);

        seqNo = pdo->seqNo;
        pdo = pdo->next;

        // Check that pdo still points to a valid range
        if (pdo < txMemBegin
                or &pdo->data >= txMemEnd
                or !pdo->type
                or pdo->type >= Pdo::End
                or (pdo->type == Pdo::Data
                    and pdo->data + pdo->count >= txMemEnd)
                or (pdo->type == Pdo::SignalList
                    and pdo->signal + pdo->count >= txMemEnd))
            pdo = txMemBegin;

        // Make sure signalListId has not changed
        if (static_cast<int>(pdo->signalListId - signalListId) > 0) {
            const Signal *signals[signalPosition.size()];
            size_t nelem;

            task->getSignalList(signals, &nelem, &signalListId);
            loadSignalList(signals, nelem, signalListId);
        }

    } while (!(!pdo->next and pdo->type == Pdo::Data
                and pdo->signalListId == signalListId));
}

////////////////////////////////////////////////////////////////////////////
bool SessionTaskData::rxPdo()
{
    while (pdo and pdo->next) {
        size_t n = pdo->count;

        switch (pdo->type) {
            case Pdo::SignalList:
                {
                    const Task::SignalVector& signals = task->getSignals();
                    const Signal *sp[signals.size()];

                    if (pdo->signal + n > txMemEnd)
                        goto out;

                    for (size_t i = 0; i < n; ++i) {
                        size_t idx = pdo->signal[i];
                        if (idx >= signals.size())
                            goto out;
                        sp[i] = static_cast<const Signal*>(signals[idx]);
                    }
                    loadSignalList(sp, n, pdo->signalListId);

                    for (size_t i = 0; i < n; ++i)
                        session->newSignal(task, sp[i]);
                }

                break;

            case Pdo::Data:
                if (pdo->data + pdoSize >= txMemEnd
                        or pdo->signalListId != signalListId
                        or pdo->seqNo - seqNo != 1)
                    goto out;

                seqNo = pdo->seqNo;
                signalBuffer = pdo->data;

                session->newSignalData(this, &pdo->time);
                break;

            default:
                goto out;
        }

        pdo = pdo->next;

        if (pdo < txMemBegin or &pdo->data > txMemEnd)
            goto out;
    }

    return false;

out:
    init();
    return true;
}

////////////////////////////////////////////////////////////////////////////
void SessionTaskData::loadSignalList(const Signal * const* sp, size_t n,
        unsigned int id)
{
//    cout << __func__ << " n=" << n << " id=" << id;
    std::fill(signalPosition.begin(),  signalPosition.end(), ~0U);
    signalListId = id;
    pdoSize = 0;
    for (size_t i = 0; i < n; ++i) {
        signalPosition[sp[i]->index] = pdoSize;
        pdoSize += sp[i]->memSize;
//        cout << ' ' << sp[i]->index << '(' << pdoSize << ')';
    }
//    cout << endl;
}

////////////////////////////////////////////////////////////////////////////
const char *SessionTaskData::getValue(const PdServ::Signal *s) const
{
    return signalBuffer + signalPosition[static_cast<const Signal*>(s)->index];
}

////////////////////////////////////////////////////////////////////////////
const struct timespec *SessionTaskData::getTaskTime() const
{
    return &pdo->time;
}

////////////////////////////////////////////////////////////////////////////
const PdServ::TaskStatistics* SessionTaskData::getTaskStatistics() const
{
    return &pdo->taskStatistics;
}
