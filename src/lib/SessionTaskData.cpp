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
#include "../SessionTask.h"
#include "SessionTaskData.h"
#include "ShmemDataStructures.h"
#include "Task.h"
#include "Signal.h"

////////////////////////////////////////////////////////////////////////////
SessionTaskData::SessionTaskData (PdServ::SessionTask *st, const Task* t,
        const Task::SignalVector& signals,
        struct Pdo *txMemBegin, const void *txMemEnd,
        const PdServ::TaskStatistics*& ts,
        const struct timespec*& time):
    task(t), sessionTask(st), signals(signals),
    txMemBegin(txMemBegin), txMemEnd(txMemEnd)
{
    signalListId = 0;
    pdoSize = 0;

    signalPosition.resize(task->signalCount());

    init();
    time = &pdo->time;
    ts = &pdo->taskStatistics;
}

////////////////////////////////////////////////////////////////////////////
SessionTaskData::~SessionTaskData ()
{
    for (SignalSet::const_iterator it = pdoSignals.begin();
            it != pdoSignals.end(); it++) {
        //log_debug("Auto unsubscribe from %s", (*it)->path.c_str());
        static_cast<const PdServ::Signal*>(*it) ->unsubscribe(sessionTask);
    }
}

////////////////////////////////////////////////////////////////////////////
// When this function exits, pdo
//      * points to the end of the pdo list,
//      * is a Data Pdo
// and its signalListId is valid
void SessionTaskData::init()
{
    const Signal *signals[signalPosition.size()];
    size_t nelem;

    pdo = txMemBegin;

    do {
        do {
            while (!pdo->next) {
                ost::Thread::sleep( static_cast<unsigned>(
                            task->sampleTime * 1000 / 2 + 1));
            }

            pdo = pdo->next;

            // Check that the pdo is valid and everyting is accessible
            if (pdo < txMemBegin
                    or &pdo->data >= txMemEnd
                    or (pdo->type != Pdo::Data
                        and pdo->type != Pdo::SignalList)
                    or (pdo->type == Pdo::Data
                        and pdo->data + pdo->count >= txMemEnd)
                    or (pdo->type == Pdo::SignalList
                        and pdo->signal + pdo->count >= txMemEnd)) {
                // Pdo is invalid. Start over
                pdo = txMemBegin;
            }
        } while (pdo->next or pdo->type != Pdo::Data);

        // At this point, we have a valid Data Pdo at the end of the list
        seqNo = pdo->seqNo;

        task->getSignalList(signals, &nelem, &signalListId);
        loadSignalList(signals, nelem, signalListId);

        log_debug("Loaded signal list with ID %u", signalListId);

    } while (signalListId != pdo->signalListId);

    log_debug("Session %p sync'ed: pdo=%p seqNo=%u signalListId=%u",
            this, (void *) pdo, seqNo, signalListId);
}

////////////////////////////////////////////////////////////////////////////
bool SessionTaskData::isBusy (const Signal *s)
{
    return pdoSignals.find(s) != pdoSignals.end();
}

////////////////////////////////////////////////////////////////////////////
bool SessionTaskData::rxPdo (const struct timespec **time,
        const PdServ::TaskStatistics **statistics)
{
    while (pdo->next) {
        size_t n;

        pdo = pdo->next;
        if (pdo < txMemBegin or &pdo->data > txMemEnd) {
            log_debug("%i\n", __LINE__);
            goto out;
        }

        n = pdo->count;

        switch (pdo->type) {
            case Pdo::SignalList:
                {
                    const Signal *sp[signals.size()];

                    if (pdo->signal + n > txMemEnd) {
                        goto out;
                    }

                    for (size_t i = 0; i < n; ++i) {
                        size_t idx = pdo->signal[i];
                        if (idx >= signals.size()) {
                            goto out;
                        }
                        sp[i] = static_cast<const Signal*>(signals[idx]);
                        sessionTask->newSignal(sp[i]);
                    }
                    loadSignalList(sp, n, pdo->signalListId);
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
                *time = &pdo->time;
                *statistics = &pdo->taskStatistics;

                return true;

            default:
                log_debug("%i\n", __LINE__);
                goto out;
        }
    }

    return false;

out:
    log_debug("Session %p out of sync.", this);
    init();
    return true;
}

////////////////////////////////////////////////////////////////////////////
void SessionTaskData::loadSignalList(const Signal * const* sp, size_t n,
        unsigned int id)
{
    log_debug("Loading %zu signals with id %u", n, id);
    //    cout << __func__ << " n=" << n << " id=" << id;
    std::fill(signalPosition.begin(),  signalPosition.end(), ~0U);
    pdoSignals.clear();

    signalListId = id;
    pdoSize = 0;
    for (size_t i = 0; i < n; ++i) {
        signalPosition[sp[i]->index] = pdoSize;
        pdoSize += sp[i]->memSize;
        pdoSignals.insert(sp[i]);
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
