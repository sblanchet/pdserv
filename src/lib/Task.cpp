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
    enum {Empty = 0, SignalList, Data, End} type;
    unsigned int signalListId;
    unsigned int count;
    struct PdServ::TaskStatistics taskStatistics;
    struct Pdo *next;
    union {
        char data[];
        const Signal *signal[];
    };
};

struct PollData {
    unsigned int request, reply;
    bool active;
    struct timespec time;
    unsigned int count;
    unsigned int length;
    const char *addr;
    struct Data {
        void *dest;
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
    std::fill_n(signalTypeCount, 4, 0);
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
    size_t n = std::accumulate(signalTypeCount, signalTypeCount + 4, 0);
    return sizeof(*signalListRp) + sizeof(*signalListWp)
        + sizeof(*poll) + n*sizeof(*poll->data)
        + 2 * n * sizeof(*signalList)
        + (sizeof(*txPdo) + signalMemSize) * (size_t)(T / sampleTime + 0.5);

}

/////////////////////////////////////////////////////////////////////////////
void Task::prepare(void *shmem, void *shmem_end)
{
    const size_t n = std::accumulate(signalTypeCount, signalTypeCount + 4, 0);

    signalListRp = ptr_align<struct SignalList*>(shmem);
    signalListWp = signalListRp + 1;
    signalList = ptr_align<struct SignalList>(signalListWp + 1);
    signalListEnd = signalList + (2*n);
    *signalListRp = signalList;
    *signalListWp = signalList;

    poll = ptr_align<struct PollData>(signalListEnd);

    txMemBegin = ptr_align<struct Pdo>(poll->data + n);
    txMemEnd = shmem_end;

    txPdo = txMemBegin;
    nextTxPdo = ptr_align<struct Pdo*>(shmem_end) - 2;

    signalPosition.resize(n);
    std::fill(signalPosition.begin(), signalPosition.end(), ~0U);
}

/////////////////////////////////////////////////////////////////////////////
void Task::rt_init()
{
    for (int i = 0; i < 4; i++) {
        copyList[i] = new struct CopyList[signalTypeCount[i] + 1];
        copyList[i]->src = 0;

        signalTypeCount[i] = 0;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Task::nrt_init()
{
    for (int i = 0; i < 4; i++)
        signalTypeCount[i] = 0;
}

/////////////////////////////////////////////////////////////////////////////
Signal* Task::addSignal( unsigned int decimation,
        const char *path, enum si_datatype_t datatype,
        const void *addr, size_t n, const unsigned int *dim)
{
    Signal *s = new Signal(this, signals.size(),
            decimation, path, datatype, addr, n, dim);

    signals.insert(s);
    signalTypeCount[s->dataTypeIndex[s->width]]++;
    signalMemSize += s->memSize;

    return s;
}

/////////////////////////////////////////////////////////////////////////////
size_t Task::signalCount() const
{
    return signals.size();
}

/////////////////////////////////////////////////////////////////////////////
void Task::getSignalList(const Signal **signalList, size_t *nelem,
        unsigned int *signalListId) const
{
    ost::SemaphoreLock lock(mutex);

    *nelem = 0;
    for (Signals::const_iterator it = signals.begin();
            it != signals.end(); ++it) {
        const Signal *s = static_cast<const Signal*>(*it);
        size_t pos = signalPosition[s->index];

        if (pos < signals.size()) {
            signalList[pos] = s;
            (*nelem)++;
        }
    }
    
    *signalListId = this->signalListId;
}

/////////////////////////////////////////////////////////////////////////////
void Task::initSession(unsigned int signalListId,
        struct Pdo **pdo, const PdServ::TaskStatistics **taskStatistics) const
{
    struct Pdo *nextPdo = txMemBegin;
    do {
        *pdo = nextPdo;
        *taskStatistics = &(*pdo)->taskStatistics;

        // Wait for a data packet
//        while (!(*pdo)->next and (*pdo)->type == Pdo::SignalList)
//        while (!(*pdo)->next and (*pdo)->type != Pdo::Data)
        while (!((*pdo)->next or (*pdo)->type == Pdo::Data))
            ost::Thread::sleep(10);

        nextPdo = nextPdo->next;
        if (!nextPdo)
            return;

        if (nextPdo < txMemBegin or &nextPdo->data >= txMemEnd
                or nextPdo->type < 1 or nextPdo->type >= Pdo::End)
            nextPdo = txMemBegin;

    } while ((nextPdo->signalListId == signalListId
                and nextPdo->type == Pdo::Data)
            or (nextPdo->signalListId <= signalListId
                and nextPdo->type == Pdo::SignalList));
}

/////////////////////////////////////////////////////////////////////////////
void Task::subscribe(const Signal* s, bool insert) const
{
//    cout << __func__ << s->index << ' ' << insert << endl;
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
void Task::pollPrepare( const Signal *signal, void *dest) const
{
    poll->data[poll->count].dest = dest;
    poll->data[poll->count].signal = signal;
    poll->length += signal->memSize;
    poll->count++;
}

/////////////////////////////////////////////////////////////////////////////
bool Task::pollFinished( const PdServ::Signal * const *s, size_t nelem,
        void * const *, struct timespec *t) const
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

    const char *buf = poll->addr;
    for (size_t i = 0; i < poll->count; ++i) {
        const Signal *s = poll->data[i].signal;
        std::copy(buf, buf + s->memSize,
                reinterpret_cast<char*>(poll->data[i].dest));
        buf += s->memSize;
    }

    poll->active = false;
    poll->count = 0;
    poll->length = 0;
    if (t)
        *t = poll->time;

    return true;
}

/////////////////////////////////////////////////////////////////////////////
void Task::update(const struct timespec *t,
        double exec_time, double cycle_time)
{
    if (poll->request != poll->reply) {
        char *dst = (txPdo->data + poll->length <= txMemEnd)
            ? txPdo->data :  txMemBegin->data;

        poll->time = *t;
        poll->addr = dst;

        for (size_t i = 0; i < poll->count; ++i) {
            const Signal *s = poll->data[i].signal;
            std::copy(s->addr, s->addr + s->memSize, dst);
            dst += s->memSize;
        }

        txPdo = ptr_align<Pdo>(dst);

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
//        cout << "Signal " << signal->index;

        switch (sp->action) {
            case SignalList::Insert:
                cl = &copyList[type][signalTypeCount[type]];
                cl->src = signal->addr;
                cl->len = signal->memSize;
                cl->signal = signal;
                cl[1].src = 0;

                pdoMem += signal->memSize;

                signalPosition[signal->index] = signalTypeCount[type]++;
//                cout << " added" << endl;
                break;

            case SignalList::Remove:
                cl = &copyList[type][signalPosition[signal->index]];
                std::copy(cl+1, copyList[type] + signalTypeCount[type]-- + 1,
                        cl);

                pdoMem -= signal->memSize;

//                cout << " removed" << endl;
                break;
        }

        *signalListRp = sp;
    }

    if (sp) {
        size_t n = std::accumulate(signalTypeCount, signalTypeCount + 4, 0);
//        cout << "New signals ";

        if ((txPdo->signal + n) > txMemEnd)
            txPdo = txMemBegin;

        txPdo->next = 0;
        txPdo->signalListId = signalListId;
        txPdo->count = n;
        txPdo->type = Pdo::SignalList;
        const Signal **sp = txPdo->signal;
        for (int i = 0; i < 4; i++) {
            for (CopyList *cl = copyList[i]; cl->src; ++cl) {
                *sp++ = cl->signal;
//                cout << ' ' << cl->signal->index;
            }
        }
//        cout << endl;

        *nextTxPdo = txPdo;

        nextTxPdo = &txPdo->next;
        txPdo = ptr_align<Pdo>(sp);
    }

    if ( txPdo->data + pdoMem >= txMemEnd)
        txPdo = txMemBegin;

    txPdo->next = 0;
    txPdo->type = Pdo::Data;
    txPdo->signalListId = signalListId;
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

    txPdo->count = p - txPdo->data;
    *nextTxPdo = txPdo;

    nextTxPdo = &txPdo->next;
    txPdo = ptr_align<Pdo>(p);
}

/////////////////////////////////////////////////////////////////////////////
bool Task::rxPdo(struct Pdo **pdoPtr, SessionTaskData *std)
{
//    cout << __func__ << endl;
    struct Pdo *pdo = *pdoPtr;

    while (pdo and pdo->next) {
        switch (pdo->type) {
            case Pdo::SignalList:
                {
//                    cout << "Pdo::SignalList " << pdo;
                    const Signal * const *sp = pdo->signal;

                    if (sp + pdo->count > txMemEnd)
                        goto out;

                    std->newSignalList( pdo->signalListId, sp, pdo->count);

                    if (pdo->signalListId > signalListId) {
                        ost::SemaphoreLock lock(mutex);

                        std::fill(signalPosition.begin(),
                                signalPosition.end(), ~0U);
                        for (size_t i = 0; i < pdo->count; ++i) {
                            signalPosition[sp[i]->index] = i;
//                            cout << ' ' << sp[i]->index;
                        }
                        signalListId = pdo->signalListId;
//                        cout << endl;
                    }
                }
                break;

            case Pdo::Data:
                {
//                    cout << "Pdo::Data " << pdo << endl;
                    if (pdo->data + pdo->count > txMemEnd)
                        goto out;

                    std->newSignalData(pdo->signalListId,
                            &pdo->taskStatistics, pdo->data, pdo->count);
                }
                break;

            default:
                goto out;
        }

        pdo = pdo->next;

        if (pdo < txMemBegin or &pdo->data > txMemEnd)
            goto out;
    }

    *pdoPtr = pdo;

    return false;

out:
    cout << "failed" << endl;
    *pdoPtr = 0;
    return true;
}
