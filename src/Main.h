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

namespace MsrProto {
    class Server;
}

namespace HRTLab {

class Session;
class Signal;
class Parameter;
class Variable;
class Task;

template <class T1>
    inline size_t element_count(size_t n)
    {
        return (n + sizeof(T1) - 1) / sizeof(T1);
    }

inline size_t element_count_uint(size_t n)
{
    return element_count<unsigned int>(n);
}

template <class T1>
    T1 align(unsigned int n)
    {
        T1 mask = sizeof(T1) - 1;
        return (n + mask) & ~mask;
    }

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
                paramupdate_t paramcheck,
                paramupdate_t paramupdate,
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

        typedef std::vector<Signal*> SignalList;
        const SignalList& getSignals() const;

        typedef std::map<const std::string,Variable*> VariableMap;
        const VariableMap& getVariableMap() const;

        // Methods used by the clients to post instructions to the real-time
        // process
        void newSession(const Session *);
        void rxPdo(Session *);
        void closeSession(const Session *s);
        void writeParameter(Parameter *);
        void poll(Signal **s, size_t nelem, char *buf);
        void unsubscribe(const Session *session,
                const Signal **s = 0, size_t nelem = 0);
        void subscribe(const Session *session,
                const Signal **s, size_t nelem);

        typedef struct {
            std::string remote;
            std::string client;
            size_t countIn;
            size_t countOut;
            struct timespec connectedTime;
        } SessionStatistics;
        void getSessionStatistics(std::list<SessionStatistics>&) const;

    private:

        mutable ost::Semaphore mutex;
        mutable ost::Semaphore pollMutex;

        MsrProto::Server *msrproto;
//    EtlProto::Server etlproto(this);

        typedef std::set<const Session*> SessionSet;
        SessionSet session;

        int pid;

        size_t shmem_len;
        char *shmem;

        struct PollStruct {
            unsigned int reqId;
            unsigned int replyId;
            unsigned int count;
            struct timespec time;
            const Signal *signals[];
        } *pollStruct;

        char *pollData;

        SignalList signals;
        ParameterList parameters;
        VariableMap variableMap;

        Task **task;

        // Methods used by the real-time process to interpret inbox
        // instructions from the clients
        void processPollSignal(unsigned int tid, const struct timespec *time);
        void processSetValue();

        static int localtime(struct timespec *);
};

}
#endif // MAIN_H
