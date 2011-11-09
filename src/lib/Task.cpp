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

#include "config.h"

#include <algorithm>
#include <numeric>

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

#include "SessionTaskData.h"
#include "Task.h"
#include "Main.h"
#include "Signal.h"
#include "Pointer.h"
#include "../TaskStatistics.h"

/////////////////////////////////////////////////////////////////////////////
// Data structures used in Task
/////////////////////////////////////////////////////////////////////////////
struct CopyList {
    const Signal *signal;
    const char *src;
    size_t len;
};

struct SignalList {
    enum {Insert = 1, Remove} action;
    unsigned int signalListId;
    const Signal* signal;
};

struct Pdo {
    enum {Empty = 0, SignalList, Data} type;
    unsigned int signalListId;
    struct PdServ::TaskStatistics taskStatistics;
    struct Pdo *next;
    char data[];
};

struct PollData {
    unsigned int request, reply;
    bool active;
    struct timespec time;
    unsigned int count;
    struct Data {
        char *dest;
        const Signal *signal;
    } data[];
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Task::Task(Main *main, double ts, const char *name):
    PdServ::Task(ts), main(main), mutex(1)
{
    pdoMem = 0;
    seqNo = 0;
    signalListId = 0;
    std::fill_n(signalCount, 4, 0);
    signalMemSize = 0;
}

/////////////////////////////////////////////////////////////////////////////
Task::~Task()
{
    delete copyList;
}

/////////////////////////////////////////////////////////////////////////////
size_t Task::getShmemSpace(double T) const
{
    size_t n = std::accumulate(signalCount, signalCount + 4, 0);
    return sizeof(*signalListRp) + sizeof(*signalListWp)
        + sizeof(*poll) + n*sizeof(*poll->data)
        + signalMemSize
        + 2 * n * sizeof(*signalList)
        + (sizeof(*txPdo) + signalMemSize) * (size_t)(T / sampleTime + 0.5);

}

/////////////////////////////////////////////////////////////////////////////
void Task::prepare(void *shmem, void *shmem_end)
{
    const size_t n = std::accumulate(signalCount, signalCount + 4, 0);

    signalListRp = ptr_align<struct SignalList*>(shmem);
    signalListWp = signalListRp + 1;
    signalList = ptr_align<struct SignalList>(signalListWp + 1);
    signalListEnd = signalList + (2*n);
    *signalListRp = signalList;
    *signalListWp = signalList;

    poll = ptr_align<struct PollData>(signalListEnd);
    pollData = ptr_align<char>(poll->data + n);

    txMemBegin = ptr_align<struct Pdo>(pollData + signalMemSize);
    txMemEnd = shmem_end;

    txPdo = txMemBegin;
    nextTxPdo = ptr_align<struct Pdo*>(shmem_end) - 2;

    signalPosition.resize(n);
}

/////////////////////////////////////////////////////////////////////////////
void Task::rt_init()
{
    for (int i = 0; i < 4; i++) {
        copyList[i] = new struct CopyList[signalCount[i] + 1];
        copyList[i]->src = 0;

        signalCount[i] = 0;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Task::nrt_init()
{
    for (int i = 0; i < 4; i++)
        signalCount[i] = 0;
}

/////////////////////////////////////////////////////////////////////////////
Signal* Task::addSignal( unsigned int decimation,
        const char *path, enum si_datatype_t datatype,
        const void *addr, size_t n, const unsigned int *dim)
{
    Signal *s = new Signal(this, signals.size(),
            decimation, path, datatype, addr, n, dim);

    signals.insert(s);
    signalCount[s->dataTypeIndex[s->width]]++;
    signalMemSize += s->memSize;

    return s;
}

/////////////////////////////////////////////////////////////////////////////
SessionTaskData *Task::newSession(PdServ::Session *s)
{
    ost::SemaphoreLock lock(mutex);

    struct Pdo *pdo = txMemBegin;
    while (pdo->next)
        pdo = pdo->next;

    const Signal *signalList[signals.size()];
    size_t i = 0;
    for (Signals::const_iterator it = signals.begin();
            it != signals.end(); ++it) {
        const Signal *s = dynamic_cast<const Signal*>(*it);
        size_t pos = signalPosition[s->index];

        if (pos != ~0U)
            signalList[i++] = s;
    }
    SessionTaskData *std = new SessionTaskData(
            this, s, pdo, signals.size(), signalListId, signalList, i);
    
    sessionMap[s] = std;

    return std;
}

/////////////////////////////////////////////////////////////////////////////
void Task::endSession(PdServ::Session *s)
{
    ost::SemaphoreLock lock(mutex);

    delete sessionMap[s];
    sessionMap.erase(s);
}

/////////////////////////////////////////////////////////////////////////////
void Task::subscribe(const Signal* s, bool insert) const
{
    ost::SemaphoreLock lock(mutex);
    struct SignalList *wp = *signalListWp;
    unsigned int signalListId = wp->signalListId + 1;

    if (++wp == signalListEnd)
        wp = signalList;

    while (wp == *signalListRp)
        ost::Thread::sleep(sampleTime * 1000 / 2 + 1);

    wp->action = insert ? SignalList::Insert : SignalList::Remove;
    wp->signal = s;
    wp->signalListId = signalListId;

    *signalListWp = wp;
}

/////////////////////////////////////////////////////////////////////////////
void Task::pollPrepare( const Signal *signal, char *dest) const
{
    poll->data[poll->count].dest = dest;
    poll->data[poll->count].signal = signal;
    poll->count++;
}

/////////////////////////////////////////////////////////////////////////////
bool Task::pollFinished( const PdServ::Signal * const *s, size_t nelem,
        char * const *pollDest, struct timespec *t) const
{
    if (!poll->count)
        return true;

    if (!poll->active) {
        poll->active = true;
        poll->request++;
        return false;
    }

    if (poll->request != poll->reply)
        return false;

    const char *buf = pollData;
    for (size_t i = 0; i < poll->count; ++i) {
        const Signal *s = poll->data[i].signal;
        std::copy(buf, buf + s->memSize, poll->data[i].dest);
        buf += s->memSize;
    }

    poll->active = false;
    poll->count = 0;
    if (t)
        *t = poll->time;

    return true;
}

/////////////////////////////////////////////////////////////////////////////
void Task::update(const struct timespec *t,
        double exec_time, double cycle_time)
{
    if (poll->request != poll->reply) {
        char *dst = pollData;
        for (size_t i = 0; i < poll->count; ++i) {
            const Signal *s = poll->data[i].signal;
            std::copy(s->addr, s->addr + s->memSize, dst);
            dst += s->memSize;
        }
        poll->time = *t;
        poll->reply = poll->request;
    }

    struct SignalList *sp = 0;

    while (*signalListRp != *signalListWp) {
        sp = *signalListRp + 1;
        if (sp == signalListEnd)
            sp = signalList;

        signalListId = sp->signalListId;
        const Signal *signal = sp->signal;
        size_t type = signal->dataTypeIndex[signal->width];
        struct CopyList *cl;

        switch (sp->action) {
            case SignalList::Insert:
                cl = &copyList[type][signalCount[type]];
                cl->src = signal->addr;
                cl->len = signal->memSize;
                cl->signal = signal;
                cl[1].src = 0;

                pdoMem += signal->memSize;

                signalPosition[signal->index] = signalCount[type]++;
                break;

            case SignalList::Remove:
                cl = &copyList[type][signalPosition[signal->index]];
                std::copy(cl+1, copyList[type] + signalCount[type]-- + 1, cl);

                pdoMem -= signal->memSize;

                break;
        }

        *signalListRp = sp;
    }

    if (sp) {
        size_t n = std::accumulate(signalCount, signalCount + 4, 0);

        if (txPdo->data + n*sizeof(Signal*) >= txMemEnd)
            txPdo = txMemBegin;

        txPdo->next = 0;
        txPdo->signalListId = signalListId;
        txPdo->taskStatistics.seqNo = n;
        txPdo->type = Pdo::SignalList;
        const Signal **sp = ptr_align<const Signal*>(txPdo->data);
        for (int i = 0; i < 4; i++) {
            for (CopyList *cl = copyList[i]; cl->src; ++cl)
                *sp++ = cl->signal;
        }

        *nextTxPdo = txPdo;

        nextTxPdo = &txPdo->next;
        txPdo = ptr_align<Pdo>(sp);
    }

    if ( txPdo->data + pdoMem >= txMemEnd)
        txPdo = txMemBegin;

    txPdo->next = 0;
    txPdo->type = Pdo::Data;
    txPdo->taskStatistics.seqNo = seqNo++;
    txPdo->taskStatistics.time = *t;
    txPdo->taskStatistics.exec_time = exec_time;
    txPdo->taskStatistics.cycle_time = cycle_time;

    char *p = txPdo->data;
    for (int i = 0; i < 4; ++i) {
        for (CopyList *cl = copyList[i]; cl->src; ++cl) {
            std::copy(cl->src, cl->src + cl->len, p);
            p += cl->len;
        }
    }

    *nextTxPdo = txPdo;

    nextTxPdo = &txPdo->next;
    txPdo = ptr_align<Pdo>(p);
}

/////////////////////////////////////////////////////////////////////////////
void Task::rxPdo(struct Pdo **pdoPtr, SessionTaskData *std)
{
    cout << __func__ << endl;
    struct Pdo *pdo = *pdoPtr;

    while (pdo->next) {
        switch (pdo->type) {
            case Pdo::SignalList:
                {
                    cout << "Pdo::SignalList" << pdo << endl;
                    const Signal * const *sp =
                        ptr_align<const Signal*>(pdo->data);
                    std->newSignalList(
                            pdo->signalListId, sp, pdo->taskStatistics.seqNo);
                    if (pdo->signalListId > signalListId) {
                        ost::SemaphoreLock lock(mutex);

                        std::fill(signalPosition.begin(),
                                signalPosition.end(), ~0U);
                        for (size_t i = 0; i < pdo->taskStatistics.seqNo; ++i)
                            signalPosition[sp[i]->index] = i;
                        signalListId = pdo->signalListId;
                    }
                }
                break;

            case Pdo::Data:
                {
                    cout << "Pdo::Data " << pdo << endl;
                    std->newSignalData(pdo->signalListId,
                            &pdo->taskStatistics, pdo->data);
                }
                break;

            default:
                break;
        }

        pdo = pdo->next;

        if (pdo < txMemBegin or (void*)pdo >= txMemEnd)
            pdo = txMemBegin;
    }

    *pdoPtr = pdo;
}
