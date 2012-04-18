/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 - 2012  Richard Hacker (lerichi at gmx dot net)
 *                         Florian Pose <fp@igh-essen.com>
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
    pdoSize = 0;

    signalPosition.resize(task->signalCount());

    init();
}

////////////////////////////////////////////////////////////////////////////
void SessionTaskData::init()
{
    pdo = txMemBegin;

    do {
        while (!pdo->next) {
            ost::Thread::sleep( static_cast<unsigned>(
                        task->sampleTime * 1000 / 2 + 1));
        }

        seqNo = pdo->seqNo;
        pdo = pdo->next;

        // Check that pdo still points to a valid range
        if (pdo < txMemBegin
                or &pdo->data >= txMemEnd
                or (pdo->type != Pdo::Data and pdo->type != Pdo::SignalList)
                or (pdo->type == Pdo::Data
                    and pdo->data + pdo->count >= txMemEnd)
                or (pdo->type == Pdo::SignalList
                    and pdo->signal + pdo->count >= txMemEnd)) {
            log_debug("Invalid PDO found. Restarting.");
            pdo = txMemBegin;
        }

        // Make sure signalListId has not changed
        if (static_cast<int>(pdo->signalListId - signalListId) > 0) {
            const Signal *signals[signalPosition.size()];
            size_t nelem;

            log_debug("Found new signal list ID: %u (was: %u)",
                    pdo->signalListId, signalListId);

            task->getSignalList(signals, &nelem, &signalListId);
            loadSignalList(signals, nelem, signalListId);

            log_debug("Loaded signal list with ID %u", signalListId);
        }

    } while (!(!pdo->next and pdo->type == Pdo::Data
                and pdo->signalListId == signalListId));

    log_debug("Session sync'ed: pdo=%p seqNo=%u signalListId=%u",
            (void *) pdo, seqNo, signalListId);
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

                    if (pdo->signal + n > txMemEnd) {
                        log_debug("%i\n", __LINE__);
                        goto out;
                    }

                    for (size_t i = 0; i < n; ++i) {
                        size_t idx = pdo->signal[i];
                        if (idx >= signals.size()) {
                            log_debug("%i\n", __LINE__);
                            goto out;
                        }
                        sp[i] = static_cast<const Signal*>(signals[idx]);
                    }
                    loadSignalList(sp, n, pdo->signalListId);

                    for (size_t i = 0; i < n; ++i) {
                        session->newSignal(task, sp[i]);
                    }
                }

                break;

            case Pdo::Data:
                if (pdo->data + pdoSize >= txMemEnd
                        or pdo->signalListId != signalListId
                        or pdo->seqNo - seqNo != 1) {
                    log_debug("%p + %zu >= %p; %u != %u; %i != 1; %u %u %u",
                            (void *) pdo->data, pdoSize, (void *) txMemEnd,
                            pdo->signalListId, signalListId,
                            pdo->seqNo - seqNo,
                            pdo->data + pdoSize >= txMemEnd,
                            pdo->signalListId != signalListId,
                            pdo->seqNo - seqNo != 1
                            );
                    goto out;
                }

                seqNo = pdo->seqNo;
                signalBuffer = pdo->data;

                session->newSignalData(this, &pdo->time);
                break;

            default:
                log_debug("%i\n", __LINE__);
                goto out;
        }

        pdo = pdo->next;

        if (pdo < txMemBegin or &pdo->data > txMemEnd) {
            log_debug("%i\n", __LINE__);
            goto out;
        }

    }

    return false;

out:
    log_debug("Session out of sync.");
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
    log_debug("pdosize=%zu", pdoSize);
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
