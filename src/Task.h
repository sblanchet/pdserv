/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef TASK_H
#define TASK_H

namespace HRTLab {

class Main;
class Session;
class Signal;

class Task {
    public:
        Task(const Main *main, unsigned int tid, double sampleTime);
        ~Task();

        const unsigned int tid;
        const double sampleTime;

//        void addSignal(const Signal *);
//        void setup();
//
//        void newSession(const Session*);
//        void endSession(const Session*);
//        void subscribe(const Session*, const Signal *);
//        void unsubscribe(const Session*, const Signal * = 0);
//        void txPdo(const struct timespec *);
//        void rxPdo(Session *);
//
    protected:
        const Main * const main;

    private:
//
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

}
#endif // TASK_H
