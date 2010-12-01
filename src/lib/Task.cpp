/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "Task.h"
#include "Main.h"
#include "Signal.h"
#include "../pointer.h"

/////////////////////////////////////////////////////////////////////////////
Task::Task(Main *main, unsigned int tid, double sampleTime):
    HRTLab::Task(main, tid, sampleTime), main(main)
{
    pdoMem = 0;
    txPdoCount = 0;
    seqNo = 0;
}

/////////////////////////////////////////////////////////////////////////////
Task::~Task()
{
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
    if (mailbox->instruction == Instruction::Clear)
        return;

    const unsigned int subscriptionSetIndex[HRTLab::Variable::maxWidth+1] = {
        0, 0, 1, 0, 2, 0, 0, 0, 3
    };

    do {
        const Signal *s = mailbox->signal;
        unsigned int idx = 3 - subscriptionSetIndex[s->width];
        SignalSet &vs = subscriptionSet[idx];
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
