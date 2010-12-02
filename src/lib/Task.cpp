/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "Task.h"
#include "Main.h"
#include "Signal.h"
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
    return (signals.size() + 1) * sizeof(Instruction)
        + std::max(10U, (size_t)(T / sampleTime + 0.5)) * pdoMem;
}

/////////////////////////////////////////////////////////////////////////////
void Task::init(void *shmem, void *shmem_end)
{
    mailboxBegin = mailbox = ptr_align<Instruction>(shmem);
    mailboxEnd = mailboxBegin + signals.size() + 1;

    txMemBegin = txFrame = ptr_align<TxFrame>(mailboxEnd);
    txMemEnd = shmem_end;
}

/////////////////////////////////////////////////////////////////////////////
void Task::subscribe(const Signal * const *s, size_t n) const
{
    ost::SemaphoreLock lock(mutex);

    if (!txPdoCount)
        pdoMem = 0;

    while (n--) {
        while (mailbox->instruction != Instruction::Clear)
            ost::Thread::sleep(100);

        txPdoCount++;
        subscriptionSet[(*s)->subscriptionIndex].insert(*s);
        mailbox->signal = *s;
        pdoMem += (*s)->memSize;
        mailbox->instruction = Instruction::Insert;

        if (++mailbox == mailboxEnd)
            mailbox = mailboxBegin;

        s++;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Task::unsubscribe(const Signal * const *s, size_t n) const
{
    ost::SemaphoreLock lock(mutex);

    while (n--) {
        while (mailbox->instruction != Instruction::Clear)
            ost::Thread::sleep(100);

        txPdoCount--;
        subscriptionSet[(*s)->subscriptionIndex].erase(*s);
        mailbox->signal = *s;
        pdoMem -= (*s)->memSize;
        mailbox->instruction = Instruction::Remove;

        if (++mailbox == mailboxEnd)
            mailbox = mailboxBegin;

        s++;
    }
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

    pdoMem += s->memSize;

    return s;
}

/////////////////////////////////////////////////////////////////////////////
void Task::receive()
{
    do {
        const Signal *s = mailbox->signal;
        SignalSet &vs = subscriptionSet[s->subscriptionIndex];
        SignalSet::iterator it = vs.find(s);

        switch (mailbox->instruction) {
            case Instruction::Insert:
                if (it == vs.end()) {
                    vs.insert(s);
                    pdoMem += s->memSize;
                    txPdoCount++;
//                    cout << "Instruction::Insert: " << v->path << endl;
                }
                break;

            case Instruction::Remove:
                if (it != vs.end()) {
                    vs.erase(it);
                    pdoMem -= s->memSize;
                    txPdoCount--;
//                    cout << "Instruction::Remove: " << v->path << endl;
                }
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
    const Signal **s = txFrame->list.signal;
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
//        cout << "rewind ";
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
