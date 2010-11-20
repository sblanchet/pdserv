/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include <sys/mman.h>

#include "Task.h"
#include "Main.h"
#include "Variable.h"
#include "Session.h"

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

using namespace HRTLab;

/////////////////////////////////////////////////////////////////////////////
    template<class T>
T* ptr_align(void *p)
{
    unsigned int mask = ~(sizeof(p) - 1);
    return reinterpret_cast<T*>(((unsigned int)p + ~mask) & mask);
}

/////////////////////////////////////////////////////////////////////////////
Task::Task(const Main *m, unsigned int tid, double sampleTime):
    tid(tid), main(m), sampleTime(sampleTime)
{
    pdoMem = 0;
    seqNo = 0;
    txPdoCount = 0;
}

/////////////////////////////////////////////////////////////////////////////
Task::~Task()
{
}

/////////////////////////////////////////////////////////////////////////////
void Task::addVariable(const Variable *v)
{
    variables.insert(v);
}

/////////////////////////////////////////////////////////////////////////////
void Task::receive()
{
//    msync(&mailbox->instruction, sizeof(mailbox->instruction), MS_INVALIDATE);

    if (mailbox->instruction == Instruction::Clear)
        return;

    const unsigned int subscriptionSetIndex[Variable::maxWidth+1] = {
        0, 0, 1, 0, 2, 0, 0, 0, 3
    };

    do {
//        msync(mailbox, sizeof(*mailbox), MS_INVALIDATE);

        const Variable *v = mailbox->variable;
        unsigned int idx = 3 - subscriptionSetIndex[v->width];
        VariableSet &vs = subscriptionSet[idx];
        VariableSet::iterator it = vs.find(v);

        switch (mailbox->instruction) {
            case Instruction::Insert:
                if (it == vs.end()) {
                    vs.insert(v);
                    pdoMem += v->memSize;
                    txPdoCount++;
                    cout << "Instruction::Insert: " << v->path << endl;
                }
                break;

            case Instruction::Remove:
                if (it != vs.end()) {
                    vs.erase(it);
                    pdoMem -= v->memSize;
                    txPdoCount--;
                    cout << "Instruction::Remove: " << v->path << endl;
                }
                break;

            default:
                break;
        }

        mailbox->instruction = Instruction::Clear;

        if (++mailbox == mailboxEnd)
            mailbox = mailboxBegin;

    } while (mailbox->instruction != Instruction::Clear);

    if (txFrame->list.variable + txPdoCount >= txMemEnd)
        txFrame = txMemBegin;

//    cout << __func__ << " L " << (void*)txFrame;
    txFrame->next = 0;
    txFrame->type = TxFrame::PdoList;
    txFrame->list.count = txPdoCount;
    const Variable **v = txFrame->list.variable;
    for (int i = 0; i < 4; i++) {
        for (VariableSet::const_iterator it = subscriptionSet[i].begin();
                it != subscriptionSet[i].end(); it++) {
//            cout << ' ' << (*it)->index;
            *v++ = *it;
        }
    }
//    cout << endl;
//    msync(txFrame, (const char*)v - (const char *)txFrame, MS_INVALIDATE);

    *nextTxFrame = txFrame;
//    msync(nextTxFrame, sizeof(*nextTxFrame), MS_INVALIDATE);

//    cout << "Setting " << nextTxFrame << " to " << txFrame << endl;
    nextTxFrame = &txFrame->next;
    txFrame = ptr_align<TxFrame>(v);
}

/////////////////////////////////////////////////////////////////////////////
void Task::deliver(Instruction::Type t, const Variable *v)
{
//    msync(mailbox, sizeof(*mailbox), MS_INVALIDATE);
    while (mailbox->instruction != Instruction::Clear) {
        ost::Thread::sleep(100);
//        msync(mailbox, sizeof(*mailbox), MS_INVALIDATE);
    }

    mailbox->instruction = t;
    mailbox->variable = v;

    if (++mailbox == mailboxEnd)
        mailbox = mailboxBegin;

    cout << __func__ << v->path<< endl;
}

/////////////////////////////////////////////////////////////////////////////
void Task::subscribe(const Session *s, const Variable *v)
{
    if (session[s].find(v) == session[s].end()) {
        cout << "Inserting " << s << v->path << endl;
        variableSessions[v].insert(s);
        session[s].insert(v);
        deliver(Instruction::Insert, v);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Task::unsubscribe(const Session *s, const Variable *v)
{
    if (v) {
        VariableSet &vs = session[s];
        VariableSet::const_iterator it = vs.find(v);

        cout << "consider rem " << s << v->path<< endl;
        if (it != vs.end()) {
            vs.erase(it);

            variableSessions[v].erase(s);
            if (variableSessions[v].empty()) {
                deliver(Instruction::Remove, v);
                cout << "removed " << s << v->path<< endl;
            }
        }
    }
    else {
        for (VariableSet::const_iterator it = variables.begin();
                it != variables.end(); it++) {
            cout << "trying to remove " << (*it)->path << endl;
            unsubscribe(s, *it);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
void Task::newSession(const Session *s)
{
    TxFrame *rxPtr = txMemBegin;
    while (rxPtr->next)
        rxPtr = rxPtr->next;

    sessionRxPtr[s] = rxPtr;
}

/////////////////////////////////////////////////////////////////////////////
void Task::endSession(const Session *s)
{
    for (VariableSet::const_iterator it = variables.begin();
            it != variables.end(); it++)
        unsubscribe(s, *it);

    sessionRxPtr.erase(sessionRxPtr.find(s));
}

/////////////////////////////////////////////////////////////////////////////
void Task::setup()
{
    size_t paramLen = variables.size() * sizeof(Instruction);
    size_t sigLen = sizeof(TxFrame);

    for (VariableSet::const_iterator it = variables.begin();
            it != variables.end(); it++) {
        sigLen += (*it)->memSize;
    }

    sigLen *= 10.0 / sampleTime;

    postoffice = ::mmap(0, paramLen + sigLen, PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_ANON, -1, 0);
    if (MAP_FAILED == postoffice) {
        // log(LOGCRIT, "could not mmap
        // err << "mmap(): " << strerror(errno);
        ::perror("mmap()");
        return;
    }
    std::fill_n((char*)postoffice, paramLen + sigLen, 0);

    mailboxBegin = reinterpret_cast<Instruction*>(postoffice);
    mailboxEnd = mailboxBegin + variables.size();
    mailboxEnd = mailboxBegin + paramLen / sizeof(Instruction);

    mailbox = mailboxBegin;

    txMemBegin = ptr_align<TxFrame>(mailboxEnd);
    txMemEnd = reinterpret_cast<const char*>((char*)postoffice + paramLen + sigLen);

    txFrame = txMemBegin;
    nextTxFrame = &txFrame[1].next;
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
        for (VariableSet::const_iterator it = subscriptionSet[i].begin();
                it != subscriptionSet[i].end(); it++) {
            const Variable *v = *it;
            std::copy(v->addr, v->addr + v->memSize, p);
            p += v->memSize;
//            cout << ' ' << v->index;
        }
    }
//    cout << endl;
//    msync(txFrame, (const char *)p - (const char *)txFrame, MS_INVALIDATE);

    *nextTxFrame = txFrame;
//    msync(nextTxFrame, sizeof(*nextTxFrame), MS_INVALIDATE);

//    cout << "Setting " << nextTxFrame << " to " << txFrame << endl;
    nextTxFrame = &txFrame->next;
    txFrame = ptr_align<TxFrame>(p);
}

/////////////////////////////////////////////////////////////////////////////
void Task::rxPdo(Session *s)
{
    size_t len;
//    TxFrame f;

    TxFrame *rxPtr = sessionRxPtr[s];
//    cout << __func__ << rxPtr << "------------" << __LINE__ << endl;
//    msync(&rxPtr->next, sizeof(rxPtr->next), MS_INVALIDATE);
    while (rxPtr->next) {
//        cout << __func__ << rxPtr << "------------" << __LINE__ << endl;
        switch (rxPtr->type) {
            case TxFrame::PdoData:
//                cout << "TxFrame::PdoData: " << rxPtr << endl;
                len = pdoMem + sizeof(*rxPtr);
//                msync(rxPtr, len, MS_INVALIDATE);
                s->newPdoData(this, rxPtr->pdo.seqNo, &rxPtr->pdo.time,
                        rxPtr->pdo.data);
//                s->newPdoData(this, f.pdo.seqNo, &f.pdo.time, b);
                break;

            case TxFrame::PdoList:
                cout << "TxFrame::PdoList: " << rxPtr << endl;
                len = (const char *)(rxPtr->list.variable + rxPtr->list.count)
                    - (const char *)rxPtr;
//                msync(rxPtr, len, MS_INVALIDATE);

                s->newVariableList(this, rxPtr->list.variable,
                        rxPtr->list.count);

                pdoMem = 0;
                for (const Variable **v = rxPtr->list.variable;
                        v != rxPtr->list.variable + rxPtr->list.count; v++)
                    pdoMem += (*v)->memSize;

                break;
        }

//        cout << "Exchanging " << rxPtr << " with " << rxPtr->next << endl;
        rxPtr = rxPtr->next;
//        cout << "  next points to " << rxPtr->next << endl;
    }

    sessionRxPtr[s] = rxPtr;
}
