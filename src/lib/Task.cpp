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
    txPdoCount = 0;
    seqNo = 0;
}

/////////////////////////////////////////////////////////////////////////////
Task::~Task()
{
    for (SignalSet::const_iterator it = signals.begin();
            it != signals.end(); it++)
        delete *it;
}

/////////////////////////////////////////////////////////////////////////////
size_t Task::getShmemSpace(double T) const
{
    size_t len = 0;
    for (SignalSet::const_iterator it = signals.begin();
            it != signals.end(); it++)
        len += (*it)->memSize;

    return (signals.size() + 1) * sizeof(Instruction)
        + std::max(10U, (size_t)(T / sampleTime + 0.5)) * len;
}

/////////////////////////////////////////////////////////////////////////////
void Task::init(void *shmem, void *shmem_end)
{
    mailboxBegin = mailbox = ptr_align<Instruction>(shmem);
    mailboxEnd = mailboxBegin + signals.size() + 1;

    txMemBegin = txFrame = ptr_align<TxFrame>(mailboxEnd);
    txMemEnd = shmem_end;

    // Start with a dummy frame
    txFrame->type = TxFrame::PdoList;
    txFrame->list.count = 0;
    nextTxFrame = &txFrame->next;
    txFrame += 1;
    txFrame->next = 0;
}

/////////////////////////////////////////////////////////////////////////////
Receiver *Task::newReceiver() const
{
    return new Receiver(tid, txMemBegin);
}

/////////////////////////////////////////////////////////////////////////////
size_t Task::subscribe(const HRTLab::Session *session,
        const HRTLab::Signal * const *s, size_t n) const
{
    ost::SemaphoreLock lock(mutex);
    size_t count = 0;

    while (n--) {
        const Signal *signal = dynamic_cast<const Signal *>(*s++);
        if (signal->task != this)
            continue;

        if (signal->sessions.empty()) {
            while (mailbox->instruction != Instruction::Clear)
                ost::Thread::sleep(sampleTime * 1000 / 2);

            txPdoCount++;
            subscriptionSet[signal->subscriptionIndex].insert(signal);
            mailbox->signal = signal;
            pdoMem += signal->memSize;
            mailbox->instruction = Instruction::Insert;

            if (++mailbox == mailboxEnd)
                mailbox = mailboxBegin;
        }

        signal->sessions.insert(session);
        count++;
    }

    return count;
}

/////////////////////////////////////////////////////////////////////////////
size_t Task::unsubscribe(const HRTLab::Session *session,
        const HRTLab::Signal * const *s, size_t n) const
{
//    cout << __PRETTY_FUNCTION__ << n << endl;
    if (!s) {
        const HRTLab::Signal *s[signals.size()];

        std::copy(signals.begin(), signals.end(), s);
        return unsubscribe(session, s, signals.size());
    }

    ost::SemaphoreLock lock(mutex);
    size_t count = 0;

    while (n--) {
        const Signal *signal = dynamic_cast<const Signal *>(*s++);
        if (signal->task != this)
            continue;

        Signal::SessionSet::const_iterator it(signal->sessions.find(session));

        if (it == signal->sessions.end())
            continue;

        signal->sessions.erase(it);
        if (signal->sessions.empty()) {
            while (mailbox->instruction != Instruction::Clear)
                ost::Thread::sleep(sampleTime * 1000 / 2);

            txPdoCount--;
            subscriptionSet[signal->subscriptionIndex].erase(signal);
            mailbox->signal = signal;
            pdoMem -= signal->memSize;
            mailbox->instruction = Instruction::Remove;

            if (++mailbox == mailboxEnd)
                mailbox = mailboxBegin;
        }

        count++;
    }

    return count;
}

/////////////////////////////////////////////////////////////////////////////
Signal *Task::newSignal(
        const char *path,
        enum si_datatype_t datatype,
        const void *addr,
        unsigned int decimation,
        unsigned int ndims,
        const unsigned int dim[])
{
    Signal *s = 
        new Signal(main, this, decimation, path, datatype, addr, ndims, dim);
    signals.insert(s);

    return s;
}

/////////////////////////////////////////////////////////////////////////////
void Task::receive()
{
    do {
        const Signal *s = mailbox->signal;
        SignalSet &vs = subscriptionSet[s->subscriptionIndex];

        switch (mailbox->instruction) {
            case Instruction::Insert:
                vs.insert(s);
                pdoMem += s->memSize;
                txPdoCount++;
//                    cout << "Instruction::Insert: " << v->path << endl;
                break;

            case Instruction::Remove:
                vs.erase(s);
                pdoMem -= s->memSize;
                txPdoCount--;
//                    cout << "Instruction::Remove: " << v->path << endl;
                break;

            default:
                break;
        }

        mailbox->instruction = Instruction::Clear;

        if (++mailbox == mailboxEnd)
            mailbox = mailboxBegin;

    } while (mailbox->instruction != Instruction::Clear);

    if (txFrame->list.signal + txPdoCount >= txMemEnd)
        txFrame = txMemBegin;

//    cout << __func__ << " L " << (void*)txFrame;
    txFrame->next = 0;
    txFrame->type = TxFrame::PdoList;
    txFrame->list.count = txPdoCount;
    const HRTLab::Signal **s = txFrame->list.signal;
    for (int i = 0; i < 4; i++) {
        for (SignalSet::const_iterator it = subscriptionSet[i].begin();
                it != subscriptionSet[i].end(); it++) {
//            cout << ' ' << (*it)->index;
            *s++ = *it;
        }
    }
//    cout << endl;

    *nextTxFrame = txFrame;

//    cout << "Setting " << nextTxFrame << " to " << txFrame << endl;
    nextTxFrame = &txFrame->next;
    txFrame = ptr_align<TxFrame>(s);
}

/////////////////////////////////////////////////////////////////////////////
void Task::txPdo(const struct timespec *t)
{
    if (mailbox->instruction != Instruction::Clear)
        receive();

    if ( txFrame->pdo.data + pdoMem >= txMemEnd) {
        txFrame = txMemBegin;
//        cout << "rewind " << endl;
    }

//    cout << __func__ << " D " << (void*)txFrame;

    txFrame->next = 0;
    txFrame->type = TxFrame::PdoData;
    txFrame->pdo.seqNo = seqNo++;
    txFrame->pdo.time = *t;

    char *p = txFrame->pdo.data;
    for (int i = 0; i < 4; i++) {
        for (SignalSet::const_iterator it = subscriptionSet[i].begin();
                it != subscriptionSet[i].end(); it++) {
            const Signal *s = *it;
            std::copy((const char*)s->addr,
                    (const char *)s->addr + s->memSize, p);
            p += s->memSize;
//            cout << ' ' << s->index;
        }
    }
//    cout << endl;

    *nextTxFrame = txFrame;

//    cout << "Setting " << nextTxFrame << " to " << txFrame << endl;
    nextTxFrame = &txFrame->next;
    txFrame = ptr_align<TxFrame>(p);
}
