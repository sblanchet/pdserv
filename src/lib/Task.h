/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef LIB_TASK_H
#define LIB_TASK_H

#include <cstddef>
#include <set>
#include <map>
#include <cc++/thread.h>

#include "../Task.h"
#include "rtlab/etl_data_info.h"
#include "ShmemDataStructures.h"

class Main;
class Signal;
class Receiver;

class Task: public HRTLab::Task {
    public:
        Task(Main *main, unsigned int tid, double sampleTime);
        ~Task();

        typedef std::set<const Signal*> SignalSet;
        void newSignal(const Signal*);
        const SignalSet& getSignalSet() const;

        size_t getShmemSpace(double t) const;
        void init(void *start, void *end);

        size_t subscribe(const HRTLab::Session*,
                const HRTLab::Signal* const *, size_t n) const;
        size_t unsubscribe(const HRTLab::Session*,
                const HRTLab::Signal* const * = 0, size_t n = 0) const;

        void txPdo(const struct timespec *);

        Receiver *newReceiver() const;

    private:
        mutable ost::Semaphore mutex;
        mutable size_t pdoMem;
        mutable size_t txPdoCount;

        SignalSet signals;
        mutable SignalSet subscriptionSet[4];

        typedef std::map<const HRTLab::Session*, SignalSet>
            SessionSubscription;
        mutable SessionSubscription sessionSubscription;

        unsigned int seqNo;
//        std::map<const Session*, SignalSet> session;
//        std::map<const Signal *, std::set<const Session*> > variableSessions;
//
//        struct Instruction {
//            enum Type {Clear, Insert, Remove} instruction;
//            const Signal *signal;
//        };
//
        Instruction *mailboxBegin, *mailboxEnd;
        mutable Instruction *mailbox;

        void mail(Instruction::Type, const Signal * = 0) const;
//
//        struct TxFrame {
//            TxFrame *next;
//            enum {PdoData = 1, PdoList} type;
//            union {
//                struct {
//                    unsigned int seqNo;
//                    struct timespec time;
//                    char data[];
//                } pdo;
//                struct {
//                    unsigned int count;
//                    const Signal *signal[];
//                } list;
//            };
//        };
//
        TxFrame *txMemBegin, *txFrame, **nextTxFrame;
        const void *txMemEnd;
//        std::map<const Session*, TxFrame *> sessionRxPtr;
//
//        void deliver(Instruction::Type t, const Signal *v);
        void receive();
};

#endif // LIB_TASK_H
