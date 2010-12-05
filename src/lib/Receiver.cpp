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

#include "Receiver.h"
#include "../Session.h"
#include "../Signal.h"

////////////////////////////////////////////////////////////////////////////
Receiver::Receiver(unsigned int tid, TxFrame *start):
    HRTLab::Receiver(tid), rxPtr(start)
{
}

////////////////////////////////////////////////////////////////////////////
Receiver::~Receiver()
{
}

////////////////////////////////////////////////////////////////////////////
const char *Receiver::getValue(const HRTLab::Signal *s) const
{
    return rxPtr->pdo.data + offset.find(s)->second;
}

////////////////////////////////////////////////////////////////////////////
void Receiver::process(HRTLab::Session *session)
{
     while (rxPtr->next) {
         switch (rxPtr->type) {
             case TxFrame::PdoData:
//                 cout << "session->newSignalData(*this);" << session << endl;
                 seqNo = rxPtr->pdo.seqNo;
                 time = &rxPtr->pdo.time;
                 session->newSignalData(*this);
                 break;
 
             case TxFrame::PdoList:
                 {
                     size_t pdoMem = 0;

                     for (size_t i = 0; i < rxPtr->list.count; i++) {
                         const HRTLab::Signal *s = rxPtr->list.signal[i];

                         offset[s] = pdoMem;
                         pdoMem += s->memSize;
                     }

//                     cout << "session->newSignalList(tid," << endl;
                     session->newSignalList(tid,
                             rxPtr->list.signal, rxPtr->list.count);
                 }
                 break;
         }
 
 //        cout << "Exchanging " << rxPtr << " with " << rxPtr->next << endl;
         rxPtr = rxPtr->next;
 //        cout << "  next points to " << rxPtr->next << endl;
     }
}


///*****************************************************************************
// * $Id$
// *****************************************************************************/
//
//#include <sys/mman.h>
//
//#include "Task.h"
//#include "Main.h"
//#include "Signal.h"
//#include "Session.h"
//
//#include <iostream>
//using std::cout;
//using std::cerr;
//using std::endl;
//
//using namespace HRTLab;
//
///////////////////////////////////////////////////////////////////////////////
//    template<class T>
//T* ptr_align(void *p)
//{
//    unsigned int mask = ~(sizeof(p) - 1);
//    return reinterpret_cast<T*>(((unsigned int)p + ~mask) & mask);
//}
//
///////////////////////////////////////////////////////////////////////////////
//Task::Task(const Main *m, unsigned int tid, double sampleTime):
//    tid(tid), main(m), sampleTime(sampleTime)
//{
//    pdoMem = 0;
//    seqNo = 0;
//    txPdoCount = 0;
//}
//
///////////////////////////////////////////////////////////////////////////////
//Task::~Task()
//{
//}
//
///////////////////////////////////////////////////////////////////////////////
//void Task::addSignal(const Signal *s)
//{
//    signals.insert(s);
//}
//
///////////////////////////////////////////////////////////////////////////////
//void Task::receive()
//{
//    if (mailbox->instruction == Instruction::Clear)
//        return;
//
//    const unsigned int subscriptionSetIndex[Variable::maxWidth+1] = {
//        0, 0, 1, 0, 2, 0, 0, 0, 3
//    };
//
//    do {
//        const Signal *s = mailbox->signal;
//        unsigned int idx = 3 - subscriptionSetIndex[s->width];
//        SignalSet &vs = subscriptionSet[idx];
//        SignalSet::iterator it = vs.find(s);
//
//        switch (mailbox->instruction) {
//            case Instruction::Insert:
//                if (it == vs.end()) {
//                    vs.insert(s);
//                    pdoMem += s->memSize;
//                    txPdoCount++;
////                    cout << "Instruction::Insert: " << v->path << endl;
//                }
//                break;
//
//            case Instruction::Remove:
//                if (it != vs.end()) {
//                    vs.erase(it);
//                    pdoMem -= s->memSize;
//                    txPdoCount--;
////                    cout << "Instruction::Remove: " << v->path << endl;
//                }
//                break;
//
//            default:
//                break;
//        }
//
//        mailbox->instruction = Instruction::Clear;
//
//        if (++mailbox == mailboxEnd)
//            mailbox = mailboxBegin;
//
//    } while (mailbox->instruction != Instruction::Clear);
//
//    if (txFrame->list.signal + txPdoCount >= txMemEnd)
//        txFrame = txMemBegin;
//
////    cout << __func__ << " L " << (void*)txFrame;
//    txFrame->next = 0;
//    txFrame->type = TxFrame::PdoList;
//    txFrame->list.count = txPdoCount;
//    const Signal **s = txFrame->list.signal;
//    for (int i = 0; i < 4; i++) {
//        for (SignalSet::const_iterator it = subscriptionSet[i].begin();
//                it != subscriptionSet[i].end(); it++) {
////            cout << ' ' << (*it)->index;
//            *s++ = *it;
//        }
//    }
////    cout << endl;
//
//    *nextTxFrame = txFrame;
//
////    cout << "Setting " << nextTxFrame << " to " << txFrame << endl;
//    nextTxFrame = &txFrame->next;
//    txFrame = ptr_align<TxFrame>(s);
//}
//
///////////////////////////////////////////////////////////////////////////////
//void Task::deliver(Instruction::Type t, const Signal *s)
//{
//    while (mailbox->instruction != Instruction::Clear) {
//        ost::Thread::sleep(100);
//    }
//
//    mailbox->instruction = t;
//    mailbox->signal = s;
//
//    if (++mailbox == mailboxEnd)
//        mailbox = mailboxBegin;
//
////    cout << __func__ << v->path<< endl;
//}
//
///////////////////////////////////////////////////////////////////////////////
//void Task::subscribe(const Session *s, const Signal *signal)
//{
//    if (session[s].find(signal) == session[s].end()) {
////        cout << "Inserting " << s << s->path << endl;
//        variableSessions[signal].insert(s);
//        session[s].insert(signal);
//        deliver(Instruction::Insert, signal);
//    }
//}
//
///////////////////////////////////////////////////////////////////////////////
//void Task::unsubscribe(const Session *s, const Signal *signal)
//{
//    if (signal) {
//        SignalSet &vs = session[s];
//        SignalSet::const_iterator it = vs.find(signal);
//
////        cout << "consider rem " << s << s->path<< endl;
//        if (it != vs.end()) {
//            vs.erase(it);
//
//            variableSessions[signal].erase(s);
//            if (variableSessions[signal].empty()) {
//                deliver(Instruction::Remove, signal);
////                cout << "removed " << s << s->path<< endl;
//            }
//        }
//    }
//    else {
//        for (SignalSet::const_iterator it = signals.begin();
//                it != signals.end(); it++) {
////            cout << "trying to remove " << (*it)->path << endl;
//            unsubscribe(s, *it);
//        }
//    }
//}
//
///////////////////////////////////////////////////////////////////////////////
//void Task::newSession(const Session *s)
//{
//    TxFrame *rxPtr = txMemBegin;
//    while (rxPtr->next)
//        rxPtr = rxPtr->next;
//
//    sessionRxPtr[s] = rxPtr;
//}
//
///////////////////////////////////////////////////////////////////////////////
//void Task::endSession(const Session *s)
//{
//    for (SignalSet::const_iterator it = signals.begin();
//            it != signals.end(); it++)
//        unsubscribe(s, *it);
//
//    sessionRxPtr.erase(sessionRxPtr.find(s));
//}
//
///////////////////////////////////////////////////////////////////////////////
//void Task::setup()
//{
//    size_t paramLen = signals.size() * sizeof(Instruction);
//    size_t sigLen = sizeof(TxFrame);
//
//    for (SignalSet::const_iterator it = signals.begin();
//            it != signals.end(); it++) {
//        sigLen += (*it)->memSize;
//    }
//
//    sigLen *= 10.0 / sampleTime;
//
//    postoffice = ::mmap(0, paramLen + sigLen, PROT_READ | PROT_WRITE,
//            MAP_SHARED | MAP_ANON, -1, 0);
//    if (MAP_FAILED == postoffice) {
//        // log(LOGCRIT, "could not mmap
//        // err << "mmap(): " << strerror(errno);
//        ::perror("mmap()");
//        return;
//    }
//    std::fill_n((char*)postoffice, paramLen + sigLen, 0);
//
//    mailboxBegin = reinterpret_cast<Instruction*>(postoffice);
//    mailboxEnd = mailboxBegin + signals.size();
//    mailboxEnd = mailboxBegin + paramLen / sizeof(Instruction);
//
//    mailbox = mailboxBegin;
//
//    txMemBegin = ptr_align<TxFrame>(mailboxEnd);
//    txMemEnd = reinterpret_cast<const char*>((char*)postoffice + paramLen + sigLen);
//
//    txFrame = txMemBegin;
//    nextTxFrame = &txFrame[1].next;
//}
//
///////////////////////////////////////////////////////////////////////////////
//void Task::txPdo(const struct timespec *t)
//{
//    receive();
//
//    if ( txFrame->pdo.data + pdoMem >= txMemEnd) {
//        txFrame = txMemBegin;
////        cout << "rewind ";
//    }
//
////    cout << __func__ << " D " << (void*)txFrame;
//
//    txFrame->next = 0;
//    txFrame->type = TxFrame::PdoData;
//    txFrame->pdo.seqNo = seqNo++;
//    txFrame->pdo.time = *t;
//
//    char *p = txFrame->pdo.data;
//    for (int i = 0; i < 4; i++) {
//        for (SignalSet::const_iterator it = subscriptionSet[i].begin();
//                it != subscriptionSet[i].end(); it++) {
//            const Signal *s = *it;
//            std::copy((const char*)s->addr,
//                    (const char *)s->addr + s->memSize, p);
//            p += s->memSize;
////            cout << ' ' << v->index;
//        }
//    }
////    cout << endl;
//
//    *nextTxFrame = txFrame;
//
////    cout << "Setting " << nextTxFrame << " to " << txFrame << endl;
//    nextTxFrame = &txFrame->next;
//    txFrame = ptr_align<TxFrame>(p);
//}
//
