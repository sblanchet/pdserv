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

#include "Task.h"
#include "Main.h"
#include "Signal.h"
#include "Receiver.h"
#include "Pointer.h"

/////////////////////////////////////////////////////////////////////////////
Task::Task(Main *main, unsigned int tid, double sampleTime):
    HRTLab::Task(main, tid, sampleTime), main(main), mutex(1)
{
    pdoMem = 0;
    seqNo = 0;
    copyList = 0;
    signalCount = 0;
    signalMemSize = 0;
    copyListId = 0;
}

/////////////////////////////////////////////////////////////////////////////
Task::~Task()
{
    delete copyList;
}

/////////////////////////////////////////////////////////////////////////////
size_t Task::getShmemSpace(double T) const
{
    return sizeof(SignalList) + signalCount * sizeof(*activeSet->list)
        +  signalMemSize * std::max(10U, (size_t)(T / sampleTime + 0.5));
}

/////////////////////////////////////////////////////////////////////////////
void Task::prepare(void *shmem, void *shmem_end)
{
    activeSet = ptr_align<SignalList>(shmem);
    activeSet->requestId = 0;
    activeSet->reply = 0;

    txMemBegin = txFrame = ptr_align<TxFrame>(activeSet->list + signalCount);
    txMemEnd = shmem_end;

    // Start with a dummy frame
    nextTxFrame = &txFrame->next;
    txFrame += 1;
    txFrame->next = 0;
}

/////////////////////////////////////////////////////////////////////////////
void Task::rt_init()
{
    copyList = new CopyList[signalCount+1];
    copyList[0].src = 0;
}

/////////////////////////////////////////////////////////////////////////////
void Task::nrt_init()
{
}

/////////////////////////////////////////////////////////////////////////////
Receiver *Task::newReceiver()
{
    ost::SemaphoreLock lock(mutex);
    Receiver *r = new Receiver(this, tid, txMemBegin);
    receiverSet.insert(r);

    return r;
}

/////////////////////////////////////////////////////////////////////////////
void Task::receiverDeleted(Receiver *r)
{
    ost::SemaphoreLock lock(mutex);
    receiverSet.erase(r);
}

/////////////////////////////////////////////////////////////////////////////
void Task::newSignalList(size_t count) const
{
    CopyList *cl = activeSet->list;
    size_t idx = 0;
    const HRTLab::Signal *signals[count];
    activeSet->pdoMemSize = 0;
    for (size_t i = 0; i < 4; i++) {
        for (SignalSet::const_iterator it = subscriptionSet[i].begin();
                it != subscriptionSet[i].end(); it++) {
            cl->src = (*it)->addr;
            cl->len = (*it)->memSize;
            cl++;

//            (*it)->offset = activeSet->pdoMemSize;
            activeSet->pdoMemSize += (*it)->memSize;

            signals[idx++] = *it;
        }
    }

    activeSet->requestId++;
    activeSet->count = count;

//    cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    for (ReceiverSet::iterator it = receiverSet.begin();
            it != receiverSet.end(); it++)
        (*it)->newSignalList(activeSet->requestId, signals, count);

    do {
        ost::Thread::sleep(sampleTime * 1000 / 2);
    } while (activeSet->requestId != activeSet->reply);
}

/////////////////////////////////////////////////////////////////////////////
size_t Task::subscribe(const HRTLab::Session *session,
        const HRTLab::Signal * const *s, size_t n) const
{
    ost::SemaphoreLock lock(mutex);
    size_t count = 0;
    bool update = false;

    while (n--) {
        const Signal *signal = dynamic_cast<const Signal *>(*s++);
        if (signal->task != this)
            continue;

        if (signal->sessions.empty()) {
//            cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
            update = true;

            subscriptionSet[signal->subscriptionIndex].insert(signal);
        }

        signal->sessions.insert(session);
        sessionSubscription[session].insert(signal);
        count++;
    }

    if (update)
        newSignalList(count);

    return count;
}

/////////////////////////////////////////////////////////////////////////////
size_t Task::unsubscribe(const HRTLab::Session *session,
        const HRTLab::Signal * const *s, size_t n) const
{
//    cout << __PRETTY_FUNCTION__ << s<< ' ' << n << endl;
    if (!s) {
        const SignalSet& subscribed = sessionSubscription[session];
        n = subscribed.size();
        const HRTLab::Signal *s[n];

        std::copy(subscribed.begin(), subscribed.end(), s);
        sessionSubscription.erase(session);
        unsubscribe(session, s, n);
        return n;
    }

    ost::SemaphoreLock lock(mutex);
    size_t count = 0;
    bool update = false;

    while (n--) {
        const Signal *signal = dynamic_cast<const Signal *>(*s++);
        if (signal->task != this)
            continue;

        sessionSubscription[session].erase(signal);
        subscriptionSet[signal->subscriptionIndex].erase(signal);

        signal->sessions.erase(session);
        if (signal->sessions.empty())
            update = true;

        count++;
    }

    if (update)
        newSignalList(count);

    return count;
}

/////////////////////////////////////////////////////////////////////////////
void Task::newSignal(const Signal* s)
{
    signalCount++;
    signalMemSize += s->memSize;
}

/////////////////////////////////////////////////////////////////////////////
void Task::txPdo(const struct timespec *t)
{
    if (activeSet->requestId != activeSet->reply) {
        std::copy(activeSet->list, activeSet->list + activeSet->count,
                copyList);
        copyList[activeSet->count].src = 0;
        pdoMem = activeSet->pdoMemSize;
        copyListId = activeSet->requestId;
        activeSet->reply = activeSet->requestId;
//        cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    }

    if ( txFrame->data + pdoMem >= txMemEnd) {
        txFrame = txMemBegin;
    }

    txFrame->next = 0;
    txFrame->signalListNo = copyListId;
    txFrame->seqNo = seqNo++;
    txFrame->time = *t;

    char *p = txFrame->data;
    for (CopyList *cl = copyList; cl->src; cl++) {
        std::copy(cl->src, cl->src + cl->len, p);
        p += cl->len;
    }

    *nextTxFrame = txFrame;

    nextTxFrame = &txFrame->next;
    txFrame = ptr_align<TxFrame>(p);
}
