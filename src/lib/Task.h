/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef LIB_TASK_H
#define LIB_TASK_H

#include "../Task.h"

class Main;
class Signal;

class Task: public HRTLab::Task {
    public:
        Task(const Main *main, unsigned int tid, double sampleTime);
        ~Task();

        const char *getValue(const HRTLab::Session *session,
                const Signal *s) const;

        void subscribe(HRTLab::Session *session, const Signal *s);
        void unsubscribe(HRTLab::Session *session, const Signal *s);
        void poll(const Signal *s, char *buf);

//        void addSignal(const Signal *);
//        void setup();
//
//        void newSession(const Session*);
//        void endSession(const Session*);
//        void subscribe(const Session*, const Signal *);
//        void unsubscribe(const Session*, const Signal * = 0);
        void txPdo(const struct timespec *);
//        void rxPdo(Session *);
//
//        const unsigned int tid;
//
    private:

        const Main * const main;
//        const double sampleTime;
//
//        typedef std::set<const Signal*> SignalSet;
//        SignalSet signals;
//        SignalSet subscriptionSet[4];
//
//        size_t pdoMem;
//        size_t txPdoCount;
//        unsigned int seqNo;
//        std::map<const Session*, SignalSet> session;
//        std::map<const Signal *, std::set<const Session*> > variableSessions;
//
//        struct Instruction {
//            enum Type {Clear, Insert, Remove} instruction;
//            const Signal *signal;
//        };
//
//        void *postoffice;
//        Instruction *mailboxBegin, *mailboxEnd, *mailbox;
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
//        TxFrame *txMemBegin, *txFrame, **nextTxFrame;
//        const void *txMemEnd;
//        std::map<const Session*, TxFrame *> sessionRxPtr;
//
//        void deliver(Instruction::Type t, const Signal *v);
//        void receive();
};

#endif // LIB_TASK_H