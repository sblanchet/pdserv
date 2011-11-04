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

#include "Task.h"
#include "Main.h"
#include "Signal.h"
#include "Pointer.h"

/////////////////////////////////////////////////////////////////////////////
// Data structures used in Task
/////////////////////////////////////////////////////////////////////////////
struct CopyList {
    const Signal *signal;
    const char *src;
    size_t len;
};

struct SignalList {
    enum {InsertSignal = 1, RemoveSignal} action;
    const Signal* signal;
};

struct TxPdo {
    enum {SignalList = 1, PdoData} type;
    unsigned int seqNo;
    struct timespec time;
    struct TxPdo *next;
    char data[];
};

struct PollData {
    unsigned int request, reply;
    struct timespec time;
    const Signal *signal[];
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Task::Task(Main *main, double ts, const char *name):
    PdServ::Task(ts), main(main), mutex(1)
{
    pollId = 0;
    pollCount = 0;
    pdoMem = 0;
    seqNo = 0;
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
        + sizeof(*poll) + (n+1)*sizeof(*poll->signal)
        + signalMemSize
        + 2 * n * sizeof(*signalList)
        + (sizeof(*txPdo) + signalMemSize) * (size_t)(T / sampleTime + 0.5);

}

/////////////////////////////////////////////////////////////////////////////
void Task::prepare(void *shmem, void *shmem_end)
{
    size_t n = std::accumulate(signalCount, signalCount + 4, 0);

    signalListRp = ptr_align<struct SignalList*>(shmem);
    signalListWp = signalListRp + 1;
    signalList = ptr_align<struct SignalList>(signalListWp + 1);
    signalListEnd = signalList + (2*n);
    *signalListRp = signalList;
    *signalListWp = signalList;

    poll = ptr_align<struct PollData>(signalListEnd);
    pollData = ptr_align<char>(poll->signal + n + 2);
    pollPtr = pollData;

    txMemBegin = ptr_align<struct TxPdo>(pollData + signalMemSize);
    txMemEnd = shmem_end;

    txPdo = txMemBegin;
    nextTxPdo = ptr_align<struct TxPdo*>(shmem_end) - 2;
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
}

/////////////////////////////////////////////////////////////////////////////
Signal* Task::addSignal( unsigned int decimation,
        const char *path, enum si_datatype_t datatype,
        const void *addr, size_t n, const unsigned int *dim)
{
    Signal *s = new Signal(this, decimation, path, datatype, addr, n, dim);

    signals.insert(s);
    signalCount[s->subscriptionIndex]++;
    signalMemSize += s->memSize;

    return s;
}

/////////////////////////////////////////////////////////////////////////////
void Task::subscribe(const Signal* s, bool insert) const
{
    ost::SemaphoreLock lock(mutex);
    struct SignalList *wp = *signalListWp;

    if (++wp == signalListEnd)
        wp = signalList;

    while (wp == *signalListRp)
        ost::Thread::sleep(sampleTime * 1000 / 2 + 1);

    wp->action = insert ? SignalList::InsertSignal : SignalList::RemoveSignal;
    wp->signal = s;

    *signalListWp = wp;
}

/////////////////////////////////////////////////////////////////////////////
void Task::pollPrepare( const Signal *signal) const
{
    poll->signal[pollCount++] = signal;
}

/////////////////////////////////////////////////////////////////////////////
bool Task::pollFinished() const
{
    if (!pollCount)
        return false;

    if (pollId == poll->request) {
        poll->signal[pollCount] = 0;
        poll->request = pollId + 1;
        return false;
    }

    if (pollId == poll->reply)
        return false;

    pollId = poll->reply;
    pollCount = 0;
    pollPtr = pollData;

    const char *buf = pollData;
    for (const Signal **s = poll->signal; *s; ++s) {
        std::copy(buf, buf + (*s)->memSize, (*s)->pollDest);
        buf += (*s)->memSize;
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////
void Task::update(const struct timespec *t)
{
    if (poll->request != poll->reply) {
        char *dst = pollData;
        for (const Signal **p = poll->signal; *p; ++p) {
            std::copy((*p)->addr, (*p)->addr + (*p)->memSize, dst);
            dst += (*p)->memSize;
        }
        poll->time = *t;
        poll->reply = poll->request;
    }

    struct SignalList *sp = 0;

    while (*signalListRp != *signalListWp) {
        sp = *signalListRp;
        const Signal *signal = sp->signal;
        size_t type = signal->subscriptionIndex;
        struct CopyList *cl;

        switch (sp->action) {
            case SignalList::InsertSignal:
                cl = &copyList[type][signalCount[type]];
                cl->src = signal->addr;
                cl->len = signal->memSize;
                cl[1].src = 0;

                pdoMem += signal->memSize;

                signal->copyListPosition = signalCount[type]++;
                break;

            case SignalList::RemoveSignal:
                cl = &copyList[type][signal->copyListPosition];
                std::copy(cl+1, copyList[type] + signalCount[type]-- + 1, cl);

                pdoMem -= signal->memSize;

                break;
        }

        if (++sp != signalListEnd)
            *signalListRp = sp;
        else
            *signalListRp = signalList;
    }

    if (sp) {
        size_t n = std::accumulate(signalCount, signalCount + 4, 0);

        if (txPdo->data + n*sizeof(Signal*) >= txMemEnd)
            txPdo = txMemBegin;

        txPdo->next = 0;
        txPdo->seqNo = n;
        txPdo->time = *t;
        const Signal **sp = ptr_align<const Signal*>(txPdo->data);
        for (int i = 0; i < 4; i++) {
            for (CopyList *cl = copyList[i]; cl->src; ++cl)
                *sp++ = cl->signal;
        }

        *nextTxPdo = txPdo;
        txPdo->type = TxPdo::SignalList;

        nextTxPdo = &txPdo->next;
        txPdo = ptr_align<TxPdo>(sp);
    }

    if ( txPdo->data + pdoMem >= txMemEnd)
        txPdo = txMemBegin;

    txPdo->next = 0;
    txPdo->seqNo = seqNo++;
    txPdo->time = *t;

    char *p = txPdo->data;
    for (int i = 0; i < 4; ++i) {
        for (CopyList *cl = copyList[i]; cl->src; ++cl) {
            std::copy(cl->src, cl->src + cl->len, p);
            p += cl->len;
        }
    }

    *nextTxPdo = txPdo;
    txPdo->type = TxPdo::PdoData;

    nextTxPdo = &txPdo->next;
    txPdo = ptr_align<TxPdo>(p);
}
