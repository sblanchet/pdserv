/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include <sys/mman.h>

#include "Task.h"
#include "Main.h"
#include "Variable.h"

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
Task::Task(const Main *m, double sampleTime):
    main(m), sampleTime(sampleTime)
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
    if (mailbox->instruction == Instruction::Clear)
        return;

    const unsigned int subscriptionSetIndex[Variable::maxWidth+1] = {
        0, 0, 1, 0, 2, 0, 0, 0, 3
    };

    do {
        switch (mailbox->instruction) {
            case Instruction::Insert:
                {
                    const Variable *v = mailbox->variable;
                    VariableSet::iterator it =
                        subscriptionSet[v->width].find(v);
                    unsigned int idx = 3 - subscriptionSetIndex[v->width];

                    if (it == subscriptionSet[v->width].end()) {
                        subscriptionSet[idx].insert(v);
                        pdoMem += v->memSize;
                        txPdoCount++;
                    }
                }
                break;

            case Instruction::Remove:
                {
                    const Variable *v = mailbox->variable;
                    VariableSet::iterator it =
                        subscriptionSet[v->width].find(v);
                    unsigned int idx = 3 - subscriptionSetIndex[v->width];

                    if (it != subscriptionSet[v->width].end()) {
                        subscriptionSet[idx].erase(it);
                        pdoMem -= v->memSize;
                        txPdoCount--;
                    }
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

    cout << __func__ << ' ' << (void*)txFrame << endl;
    txFrame->next = 0;
    txFrame->type = TxFrame::PdoList;
    txFrame->list.count = txPdoCount;
    const Variable **v = txFrame->list.variable;
    for (int i = 0; i < 4; i++) {
        for (VariableSet::const_iterator it = subscriptionSet[i].begin();
                it != subscriptionSet[i].end(); it++) {
            *v++ = *it;
        }
    }

    *nextTxFrame = txFrame;
    nextTxFrame = &txFrame->next;
    txFrame = ptr_align<TxFrame>(v);
}

/////////////////////////////////////////////////////////////////////////////
void Task::deliver(Instruction::Type t, const Variable *v)
{
    while (mailbox->instruction != Instruction::Clear)
        sleep(1);

    mailbox->instruction = t;
    mailbox->variable = v;

    if (++mailbox == mailboxEnd)
        mailbox = mailboxBegin;
}

/////////////////////////////////////////////////////////////////////////////
void Task::subscribe(const Session *s, const Variable *v)
{
    if (session[s].find(v) == session[s].end()) {
        session[s].insert(v);
        deliver(Instruction::Insert, v);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Task::unsubscribe(const Session *s, const Variable *v)
{
    if (session[s].find(v) != session[s].end()) {
        session[s].erase(v);
        if (session[s].empty())
            deliver(Instruction::Remove, v);
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
    size_t n = 0;

    for (VariableSet::const_iterator it = variables.begin();
            it != variables.end(); it++) {
        n += (*it)->memSize;
    }

    n *= 2.0 / sampleTime;
    n += variables.size() * sizeof(Instruction);

    postoffice = ::mmap(0, n, PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_ANON, -1, 0);
    if (MAP_FAILED == postoffice) {
        // log(LOGCRIT, "could not mmap
        // err << "mmap(): " << strerror(errno);
        ::perror("mmap()");
        return;
    }
    std::fill_n((char*)postoffice, n, 0);

    mailboxBegin = reinterpret_cast<Instruction*>(postoffice);
    mailboxEnd = mailboxBegin + variables.size();

    mailbox = mailboxBegin;

    txMemBegin = ptr_align<TxFrame>(mailboxEnd);
    txMemEnd = reinterpret_cast<const char*>((char*)postoffice + n);

    txFrame = txMemBegin;
    nextTxFrame = &txFrame[1].next;
}

/////////////////////////////////////////////////////////////////////////////
void Task::txPdo(const struct timespec *t)
{

    receive();

    if ( txFrame->pdo.data + pdoMem >= txMemEnd) {
        txFrame = txMemBegin;
        cout << "rewind ";
    }

    cout << __func__ << ' ' << (void*)txFrame << endl;

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
        }
    }

    *nextTxFrame = txFrame;
    nextTxFrame = &txFrame->next;
    txFrame = ptr_align<TxFrame>(p);
}

/////////////////////////////////////////////////////////////////////////////
void Task::rxPdo(Session *s)
{
    TxFrame *rxPtr = sessionRxPtr[s];
    while (rxPtr->next) {
        switch (rxPtr->type) {
            case TxFrame::PdoData:
                cout << "TxFrame::PdoData: " << (void*)rxPtr << endl;
                break;

            case TxFrame::PdoList:
                cout << "TxFrame::PdoList: " << (void*)rxPtr << endl;
                break;
        }

        rxPtr = rxPtr->next;
    }

    sessionRxPtr[s] = rxPtr;
}
