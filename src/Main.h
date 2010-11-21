#ifndef MAIN_H
#define MAIN_H

#include <vector>
#include <map>
#include <set>
#include <list>
#include <algorithm>
#include <string>
#include <stdint.h>

#include <rtlab/rtlab.h>
#include <cc++/thread.h>

#include "Session.h"

namespace MsrProto {
    class Server;
}

namespace HRTLab {

class Signal;
class Parameter;
class Variable;
class Task;

class Main {
    public:
        enum Instruction {Restart = 1,
            Subscribe, Unsubscribe, SetValue,
            PollSignal,

            SubscriptionList, SubscriptionData,
        };

        Main(int argc, const char *argv[],
                const char *name, const char *version,
                double baserate,
                unsigned int nst, const unsigned int decimation[],
                int (*gettime)(struct timespec*));
        ~Main();

        void update(int st, const struct timespec *time);
        int start();

        int newSignal(
                unsigned int tid,
                unsigned int decimation,
                const char *path,
                const char *alias,
                enum si_datatype_t datatype,
                unsigned int ndims,
                const unsigned int dim[],
                const char *addr
                );

        int newParameter(
                const char *path,
                const char *alias,
                enum si_datatype_t datatype,
                unsigned int ndims,
                const unsigned int dim[],
                char *addr,
                paramupdate_t paramcopy,
                void *priv_data
                );

        const std::string name;
        const std::string version;
        const double baserate;
        const unsigned int nst;
        unsigned int * const decimation;
        int (* const gettime)(struct timespec*);

        typedef std::vector<Parameter*> ParameterList;
        const ParameterList& getParameters() const;
        const struct timespec *getMTime(const Parameter *) const;
        const char *getParameterAddr(const Parameter *) const;

        typedef std::vector<Signal*> SignalList;
        const SignalList& getSignals() const;

        typedef std::map<const std::string,Variable*> VariableMap;
        const VariableMap& getVariableMap() const;

        // Methods used by the clients to post instructions to the real-time
        // process
        void newSession(const Session *);
        void rxPdo(Session *);
        void closeSession(const Session *s);
        int writeParameter(const Parameter * const *, size_t n,
                const char *data);
        int poll(const Signal * const *s, size_t nelem, char *buf);
        void unsubscribe(const Session *session,
                const Signal * const *s = 0, size_t nelem = 0);
        void subscribe(const Session *session,
                const Signal * const *s, size_t nelem);

        void getSessionStatistics(std::list<Session::Statistics>&) const;

    private:

        mutable ost::Semaphore mutex;

        MsrProto::Server *msrproto;
//    EtlProto::Server etlproto(this);

        int pid;

        size_t shmem_len;
        void *shmem;

        struct SDOStruct {
            unsigned int reqId;
            unsigned int replyId;
            enum {PollSignal, WriteParameter} type;
            int errorCode;
            unsigned int count;
            struct timespec time;
            union {
                const Signal *signals[];
                const Parameter *parameters[];
            };
        } *sdo;
        mutable ost::Semaphore sdoMutex;
        char *sdoData;
        char *parameterData;
        char **parameterAddr;

        struct timespec *mtime;

        SignalList signals;
        ParameterList parameters;
        VariableMap variableMap;

        Task **task;

        // Methods used by the real-time process to interpret inbox
        // instructions from the clients
        void processSdo(unsigned int tid, const struct timespec *time);

        static int localtime(struct timespec *);
};

}
#endif // MAIN_H
