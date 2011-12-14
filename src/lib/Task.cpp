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

#include <algorithm>
#include <numeric>

#include "ShmemDataStructures.h"
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
Task::Task(Main *main, double ts, const char *name):
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
    delete copyList[0];
    delete signalCopyList[0];
    delete signalPosition;
}

/////////////////////////////////////////////////////////////////////////////
size_t Task::getShmemSpace(double T) const
{
    size_t n = signals.size();
    return sizeof(*signalListRp) + sizeof(*signalListWp)
        + sizeof(*poll) + n*sizeof(*poll->data)
        + 2 * n * sizeof(*signalList)
        + (sizeof(*txPdo) + signalMemSize) * (size_t)(T / sampleTime + 0.5);

}

/////////////////////////////////////////////////////////////////////////////
SessionTaskData* Task::newSession(PdServ::Session *session)
{
    return new SessionTaskData(session, this, txMemBegin, txMemEnd);
}

/////////////////////////////////////////////////////////////////////////////
void Task::prepare(void *shmem, void *shmem_end)
{
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
    debug() << "signallen=" << signalMemSize << " txpdosize=" 
        << sizeof(*txPdo) << " space="
        << ((const char*)txMemEnd - (const char *)txMemBegin);

    txPdo = txMemBegin;
    nextTxPdo = ptr_align<struct Pdo*>(shmem_end) - 2;
}

/////////////////////////////////////////////////////////////////////////////
void Task::rt_init()
{
    signalMemSize = 0;

    copyList[0] = new struct CopyList[signals.size() + 4];
    for (int i = 1; i < 4; i++)
        copyList[i] = copyList[i-1] + signalTypeCount[i-1] + 1;

    std::fill_n(signalTypeCount, 4, 0);
    for (size_t i = 0; i < signals.size() + 4; ++i)
        copyList[0][i].src = 0;
}

/////////////////////////////////////////////////////////////////////////////
void Task::nrt_init()
{
    signalVector.resize(signals.size());
    for (Signals::const_iterator it = signals.begin();
            it != signals.end(); ++it) {
        const Signal *s = static_cast<const Signal*>(*it);
        signalVector[s->index] = s;
    }

    signalPosition = new unsigned int[signals.size()];

    signalCopyList[0] = new const Signal*[signals.size() + 4];
    for (int i = 1; i < 4; i++)
        signalCopyList[i] = signalCopyList[i-1] + signalTypeCount[i-1] + 1;

    std::fill_n(signalTypeCount, 4, 0);
    std::fill_n(signalCopyList[0], signals.size() + 4,
            reinterpret_cast<Signal const*>(0));
}

/////////////////////////////////////////////////////////////////////////////
Signal* Task::addSignal( unsigned int decimation,
        const char *path, PdServ::Variable::Datatype datatype,
        const void *addr, size_t n, const size_t *dim)
{
    Signal *s = new Signal(this, signals.size(),
            decimation, path, datatype, addr, n, dim);

    signals.push_back(s);
    signalTypeCount[s->dataTypeIndex[s->width]]++;
    signalMemSize += s->memSize;

    return s;
}

/////////////////////////////////////////////////////////////////////////////
const Task::SignalVector& Task::getSignals() const
{
    return signalVector;
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

    for (unsigned int i = 0; i < 4; ++i)
        for (unsigned int j = 0; j < signalTypeCount[i]; ++j)
            *signalList++ = signalCopyList[i][j];
    *nelem = std::accumulate(signalTypeCount, signalTypeCount + 4, 0);
    *signalListId = (*signalListWp)->signalListId;
}

/////////////////////////////////////////////////////////////////////////////
void Task::subscribe(const Signal* s, bool insert)
{
    ost::SemaphoreLock lock(mutex);
    struct SignalList *wp = *signalListWp;

    if (++wp == signalListEnd)
        wp = signalList;

    while (wp == *signalListRp)
        ost::Thread::sleep(sampleTime * 1000 / 2 + 1);

//    debug() << insert << s->path;
    size_t w = s->dataTypeIndex[s->width];
    if (insert) {
        size_t i = signalTypeCount[w]++;

        wp->action = SignalList::Insert;
        wp->signal = s;
        wp->signalPosition = i;

        signalCopyList[w][i] = s;
        signalPosition[s->index] = i;
//        debug() << "insert" << s->path << w << i << (void*)signalCopyList[w];
    }
    else {
        wp->action = SignalList::Remove;
        wp->signalPosition = signalPosition[s->index];

        const Signal **dst = signalCopyList[w] + signalPosition[s->index];
//        debug() << "erase" << s->path << w << signalPosition[s->index]
//            << "copy("
//            << (void*)(signalCopyList[w])
//            << signalPosition[s->index] + 1
//            << signalTypeCount[w]
//            << signalPosition[s->index];
        std::copy( dst + 1, signalCopyList[w] + signalTypeCount[w]-- + 1, dst);

        for (; *dst; ++dst) {
            --signalPosition[(*dst)->index];
//            debug() << "position for" << (*dst)->path
//                << signalPosition[(*dst)->index];
        }
    }
    wp->signal = s;
    wp->signalListId = ++signalListId;
//    cout << __func__ << s->index << ' ' << insert
//        << " pos=" << wp->signalPosition << endl;

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
void Task::updateStatistics(
        double exec_time, double cycle_time, unsigned int overrun)
{
    taskStatistics.exec_time = exec_time;
    taskStatistics.cycle_time = cycle_time;
    taskStatistics.overrun = overrun;
}

/////////////////////////////////////////////////////////////////////////////
void Task::update(const struct timespec *t)
{
    if (poll->request != poll->reply) {
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

    struct SignalList *sp = 0;

    while (*signalListRp != *signalListWp) {
        sp = *signalListRp + 1;
        if (sp == signalListEnd)
            sp = signalList;

        signalListId = sp->signalListId;
        const Signal *signal = sp->signal;
        size_t type = signal->dataTypeIndex[signal->width];
        struct CopyList *cl = &copyList[type][sp->signalPosition];
//        cout << "Signal " << signal->index << " pos=" << sp->signalPosition;

        switch (sp->action) {
            case SignalList::Insert:
                cl->src = signal->addr;
                cl->len = signal->memSize;
                cl->signal = signal;

                signalMemSize += signal->memSize;
                signalTypeCount[type]++;

//                cout << " added" << endl;
                break;

            case SignalList::Remove:
                std::copy(cl+1, copyList[type] + signalTypeCount[type]-- + 1,
                        cl);

                signalMemSize -= signal->memSize;

//                cout << " removed" << endl;
                break;
        }

        *signalListRp = sp;
    }
//    cout << sp << endl;

    if (sp) {
        size_t n = std::accumulate(signalTypeCount, signalTypeCount + 4, 0);
//        cout << "New signals " << n << ' ' << "signalMemSize=" << signalMemSize;

        if ((txPdo->signal + n) > txMemEnd)
            txPdo = txMemBegin;

        txPdo->next = 0;
        txPdo->signalListId = signalListId;
        txPdo->count = n;
        txPdo->type = Pdo::SignalList;
        size_t *sp = txPdo->signal;
        for (int i = 0; i < 4; i++)
            for (CopyList *cl = copyList[i]; cl->src; ++cl) {
                *sp++ = cl->signal->index;
                std::cerr << ' ' << cl->signal->index;
            }
        std::cerr << std::endl;
//        cout << endl;

        *nextTxPdo = txPdo;

        nextTxPdo = &txPdo->next;
        txPdo = ptr_align<Pdo>(sp);
    }

    if ( txPdo->data + signalMemSize >= txMemEnd) {
        txPdo = txMemBegin;
//        cout << "wrap" << endl;
    }

    txPdo->next = 0;
    txPdo->type = Pdo::Data;
    txPdo->signalListId = signalListId;
    txPdo->seqNo = seqNo++;

    if (t)
        txPdo->time = *t;
    else
        txPdo->time.tv_sec = txPdo->time.tv_nsec = 0;

    txPdo->taskStatistics = taskStatistics;

    char *p = txPdo->data;
    for (int i = 0; i < 4; ++i) {
//        cout << i << ": ";
        for (CopyList *cl = copyList[i]; cl->src; ++cl) {
            std::copy(cl->src, cl->src + cl->len, p);
            p += cl->len;
//            cout << cl->signal->index << ' ' << cl->len << ' ';
        }
    }
//    cout << '=' << p - txPdo->data << endl;

    txPdo->count = p - txPdo->data;
    *nextTxPdo = txPdo;

    nextTxPdo = &txPdo->next;
    txPdo = ptr_align<Pdo>(p);
}
