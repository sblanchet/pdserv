#ifndef TASK_H
#define TASK_H

#include <set>
#include <map>
#include "Variable.h"

namespace HRTLab {

class Main;
class Session;

class Task {
    public:
        Task(const Main *main, double sampleTime);
        ~Task();

        void addVariable(const Variable *);
        void setup();

        void newSession(const Session*);
        void endSession(const Session*);
        void subscribe(const Session*, const Variable *);
        void unsubscribe(const Session*, const Variable *);
        void txPdo(const struct timespec *);
        void rxPdo(Session *);

    private:

        const Main * const main;
        const double sampleTime;

        typedef std::set<const Variable*> VariableSet;
        VariableSet variables;
        VariableSet subscriptionSet[4];

        size_t pdoMem;
        size_t txPdoCount;
        unsigned int seqNo;
        std::map<const Session*, std::set<const Variable*> > session;

        struct Instruction {
            enum Type {Clear, Insert, Remove} instruction;
            const Variable *variable;
        };

        void *postoffice;
        Instruction *mailboxBegin, *mailboxEnd, *mailbox;

        struct TxFrame {
            TxFrame *next;
            enum {PdoData = 1, PdoList} type;
            union {
                struct {
                    unsigned int seqNo;
                    struct timespec time;
                    char data[];
                } pdo;
                struct {
                    unsigned int count;
                    const Variable *variable[];
                } list;
            };
        };

        TxFrame *txMemBegin, *txFrame, **nextTxFrame;
        const void *txMemEnd;
        std::map<const Session*, TxFrame *> sessionRxPtr;

        void deliver(Instruction::Type t, const Variable *v);
        void receive();
};

}
#endif // TASK_H
