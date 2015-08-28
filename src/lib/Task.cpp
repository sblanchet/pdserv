/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
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

#include <algorithm>
#include <numeric>

#include "ShmemDataStructures.h"
#include "SessionTaskData.h"
#include "../SessionTask.h"
#include "Task.h"
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
    enum {Insert = 1, Remove} action;
    unsigned int signalListId;
    unsigned int signalPosition;
    const Signal* signal;
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
Task::Task(Main *main, double ts, const char * /*name*/):
    PdServ::Task(ts), main(main), mutex(1)
{
    seqNo = 0;
    signalMemSize = 0;
    signalPosition = 0;
    signalListId = 0;
    std::fill_n(signalTypeCount, 4, 0);
    signalCopyList[0] = 0;
    copyList[0] = 0;
}

/////////////////////////////////////////////////////////////////////////////
Task::~Task()
{
    delete[] copyList[0];
    delete[] signalCopyList[0];
    delete[] signalPosition;

    for (size_t i = 0; i < signals.size(); ++i)
        delete signals[i];
}

/////////////////////////////////////////////////////////////////////////////
Signal* Task::addSignal( unsigned int decimation,
        const char *path, const PdServ::DataType& datatype,
        const void *addr, size_t n, const size_t *dim)
{
    Signal *s = new Signal(this, signals.size(),
            decimation, path, datatype, addr, n, dim);

    signals.push_back(s);
    signalTypeCount[s->dataTypeIndex[s->dtype.align()]]++;
    signalMemSize += s->memSize;

    return s;
}

/////////////////////////////////////////////////////////////////////////////
std::list<const PdServ::Signal*> Task::getSignals() const
{
    return std::list<const PdServ::Signal*>(signals.begin(), signals.end());
}

/////////////////////////////////////////////////////////////////////////////
// Initialization methods
/////////////////////////////////////////////////////////////////////////////
size_t Task::getShmemSpace(double T) const
{
    size_t n = signals.size();
    size_t minPdoCount = (size_t)(T / sampleTime + 0.5);

    if (minPdoCount < 10)
        minPdoCount = 10;

    return sizeof(*signalListRp) + sizeof(*signalListWp)
        + sizeof(*poll) + n*sizeof(*poll->data)
        + 2 * n * sizeof(*signalList)
        + (sizeof(*txPdo) + signalMemSize) * minPdoCount;
}

/////////////////////////////////////////////////////////////////////////////
void Task::prepare(void *shmem, void *shmem_end)
{
    //log_debug("S(%p): shmem=%p shmem_end=%p", this, shmem, shmem_end);
    size_t n = signals.size();

    signalListRp = ptr_align<struct SignalList*>(shmem);
    signalListWp = signalListRp + 1;
    signalList = ptr_align<struct SignalList>(signalListWp + 1);
    signalListEnd = signalList + (2*n);
    *signalListRp = signalList;
    *signalListWp = signalList;

    poll = ptr_align<struct PollData>(signalListEnd);

    txMemBegin = ptr_align<struct Pdo>(poll->data + n);
    txMemEnd = shmem_end;
    //log_debug("S(%p): txMemBegin=%p", this, txMemBegin);

    txPdo = txMemBegin;
    nextTxPdo = ptr_align<struct Pdo*>(shmem_end) - 2;
}

/////////////////////////////////////////////////////////////////////////////
void Task::rt_init()
{
    signalMemSize = 0;

    copyList[0] = new struct CopyList[signals.size() + 4];
    for (size_t i = 0; i < 3; i++)
        copyList[i+1] = copyList[i] + signalTypeCount[i] + 1;

    std::fill_n(signalTypeCount, 4, 0);

    // Clear src field which is end-of-list marker
    for (size_t i = 0; i < signals.size() + 4; ++i)
        copyList[0][i].src = 0;
}

/////////////////////////////////////////////////////////////////////////////
void Task::nrt_init()
{
    signalPosition = new unsigned int[signals.size()];

    signalCopyList[0] = new const Signal*[signals.size()];
    for (size_t i = 0; i < 3; i++)
        signalCopyList[i+1] = signalCopyList[i] + signalTypeCount[i];

    std::fill_n(signalTypeCount, 4, 0);
}

/////////////////////////////////////////////////////////////////////////////
// Non-real time methods
/////////////////////////////////////////////////////////////////////////////
void Task::getSignalList(const Signal **signalList, size_t *nelem,
        unsigned int *signalListId) const
{
    ost::SemaphoreLock lock(mutex);

    for (unsigned int i = 0; i < 4; ++i)
        for (unsigned int j = 0; j < signalTypeCount[i]; ++j)
            *signalList++ = signalCopyList[i][j];
    *nelem = std::accumulate(signalTypeCount, signalTypeCount + 4, 0);
    *signalListId = (*signalListWp)->signalListId;
}

/////////////////////////////////////////////////////////////////////////////
void Task::subscribe(const Signal* s, bool insert) const
{
    ost::SemaphoreLock lock(mutex);
    struct SignalList *wp = *signalListWp;

    if (++wp == signalListEnd)
        wp = signalList;

    while (wp == *signalListRp)
        ost::Thread::sleep(static_cast<unsigned>(sampleTime * 1000 / 2 + 1));

    size_t w = s->dataTypeIndex[s->dtype.align()];
    const Signal **scl = signalCopyList[w];

    wp->signal = s;

    if (insert) {
        wp->action = SignalList::Insert;

        size_t i = signalTypeCount[w]++;
        scl[i] = s;
        signalPosition[s->index] = i;
    }
    else {
        size_t pos = signalPosition[s->index];

        wp->action = SignalList::Remove;
        wp->signalPosition = pos;

        // Replace s with last signal on the list
        s = scl[--signalTypeCount[w]];
        scl[pos] = s;
        signalPosition[s->index] = pos;
    }

    wp->signalListId = ++signalListId;

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
bool Task::pollFinished( const PdServ::Signal * const *, size_t /*nelem*/,
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
void Task::updateStatistics(
        double exec_time, double cycle_time, unsigned int overrun)
{
    taskStatistics.exec_time = exec_time;
    taskStatistics.cycle_time = cycle_time;
    taskStatistics.overrun = overrun;
}

/////////////////////////////////////////////////////////////////////////////
void Task::prepare (PdServ::SessionTask *s) const
{
    s->sessionTaskData =
        new SessionTaskData(s, this, &signals, txMemBegin, txMemEnd);
}

/////////////////////////////////////////////////////////////////////////////
void Task::cleanup (const PdServ::SessionTask *s) const
{
    delete s->sessionTaskData;
}

/////////////////////////////////////////////////////////////////////////////
bool Task::rxPdo (PdServ::SessionTask *s, const struct timespec **time,
        const PdServ::TaskStatistics **stat) const
{
    return s->sessionTaskData->rxPdo(time, stat);
}

/////////////////////////////////////////////////////////////////////////////
// Real time methods
/////////////////////////////////////////////////////////////////////////////
void Task::update(const struct timespec *t)
{
    while (poll->request != poll->reply)
        processPollRequest(t);

    if (*signalListRp != *signalListWp) {
        while (*signalListRp != *signalListWp)
            processSignalList();

        calculateCopyList();
    }

    copyData(t);
}

/////////////////////////////////////////////////////////////////////////////
void Task::processPollRequest(const struct timespec *t)
{
    // Rewind pointers
    if (txPdo->data + poll->length >= txMemEnd) {
        txPdo = txMemBegin;
        txPdo->type = Pdo::Empty;
        txPdo->next = 0;
    }

    char *dst = txPdo->data;

    if (t)
        poll->time = *t;
    else
        poll->time.tv_sec = poll->time.tv_nsec = 0;

    poll->addr = dst;

    for (size_t i = 0; i < poll->count; ++i) {
        const Signal *s = poll->data[i].signal;
        std::copy(s->addr, s->addr + s->memSize, dst);
        dst += s->memSize;
    }

    txPdo = ptr_align<Pdo>(dst);

    if (poll->addr == txMemBegin->data) {
        txPdo->next = 0;
        txPdo->type = Pdo::Empty;
        txMemBegin->next = txPdo;
    }

    poll->reply = poll->request;
}

/////////////////////////////////////////////////////////////////////////////
void Task::processSignalList()
{
    struct SignalList *sp = *signalListRp + 1;

    if (sp == signalListEnd)
        sp = signalList;

    signalListId = sp->signalListId;
    const Signal *signal = sp->signal;
    size_t w = signal->dataTypeIndex[signal->dtype.align()];
    struct CopyList *cl;

    switch (sp->action) {
        case SignalList::Insert:
            // Insert the signal at list end
            cl = copyList[w] + signalTypeCount[w]++;

            cl->src = signal->addr;
            cl->len = signal->memSize;
            cl->signal = signal;

            signalMemSize += signal->memSize;
            break;

        case SignalList::Remove:
            // Move signal at list end to the deleted position
            cl = copyList[w] + --signalTypeCount[w];
            copyList[w][sp->signalPosition] = *cl;
            cl->src = 0;    // End of copy list indicator

            signalMemSize -= signal->memSize;
            break;
    }

    *signalListRp = sp;
}

/////////////////////////////////////////////////////////////////////////////
void Task::calculateCopyList()
{
    size_t n = std::accumulate(signalTypeCount, signalTypeCount + 4, 0);

    if ((txPdo->signal + n) >= txMemEnd)
        txPdo = txMemBegin;

    txPdo->next = 0;
    txPdo->type = Pdo::Empty;
    txPdo->signalListId = signalListId;
    txPdo->count = n;
    size_t *sp = txPdo->signal;
    for (int i = 0; i < 4; i++) {
        for (CopyList *cl = copyList[i]; cl->src; ++cl)
            *sp++ = cl->signal->index;
    }
    txPdo->type = Pdo::SignalList;

    *nextTxPdo = txPdo;

    nextTxPdo = &txPdo->next;
    txPdo = ptr_align<Pdo>(sp);
}

/////////////////////////////////////////////////////////////////////////////
void Task::copyData(const struct timespec *t)
{
    if ( txPdo->data + signalMemSize >= txMemEnd)
        txPdo = txMemBegin;

    txPdo->next = 0;
    txPdo->type = Pdo::Empty;
    txPdo->signalListId = signalListId;
    txPdo->seqNo = seqNo++;

    txPdo->taskStatistics = taskStatistics;

    if (t) {
        txPdo->time = *t;
    }
    else
        txPdo->time.tv_sec = txPdo->time.tv_nsec = 0;

    char *p = txPdo->data;
    for (int i = 0; i < 4; ++i) {
        for (CopyList *cl = copyList[i]; cl->src; ++cl) {
            std::copy(cl->src, cl->src + cl->len, p);
            p += cl->len;
        }
    }
    txPdo->type = Pdo::Data;

    txPdo->count = p - txPdo->data;
    *nextTxPdo = txPdo;

    nextTxPdo = &txPdo->next;
    txPdo = ptr_align<Pdo>(p);
}
