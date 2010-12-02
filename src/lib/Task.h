/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef LIB_TASK_H
#define LIB_TASK_H

#include <cstddef>
#include <set>
#include <cc++/thread.h>

#include "../Task.h"
#include "rtlab/etl_data_info.h"
#include "ShmemDataStructures.h"

class Main;
class Signal;

class Task: public HRTLab::Task {
    public:
        Task(Main *main, unsigned int tid, double sampleTime);
        ~Task();

        Signal *newSignal(
                const char *path,
                enum si_datatype_t datatype,
                const void *addr,
                unsigned int decimation,
                unsigned int ndims = 1,
                const unsigned int dim[] = 0);

        size_t getShmemSpace(double t) const;
        void init(void *);

        void poll(const Signal *s, char *buf, struct timespec *) const;

        void subscribe(const Signal* const *, size_t n) const;
        void unsubscribe(const Signal* const *, size_t n) const;

        void txPdo(const struct timespec *);

    private:

        Main * const main;

        mutable ost::Semaphore mutex;
        mutable size_t pdoMem;
        mutable size_t txPdoCount;

        typedef std::set<const Signal*> SignalSet;
        SignalSet signals;
        mutable SignalSet subscriptionSet[4];

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
