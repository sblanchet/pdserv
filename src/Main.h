#ifndef MAIN_H
#define MAIN_H

#include <vector>
#include <map>
#include <algorithm>
#include <string>
#include <stdint.h>

#include <rtlab/rtlab.h>
#include <cc++/thread.h>

namespace MsrProto {
    class Server;
}

namespace HRTLab {

class Signal;
class Parameter;
class Variable;

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
            GetSubscriptionList, Subscribe, Unsubscribe, SetValue,
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


        unsigned int *getSignalPtrStart() const;

        // Methods used by the clients to post instructions to the real-time
        // process
        void writeParameter(Parameter *);
        void poll(const SignalList&);
        void subscribe(const Signal **s, size_t nelem);
        void subscriptionList();

    private:

        mutable ost::Semaphore mutex;
        MsrProto::Server *msrproto;
//    EtlProto::Server etlproto(this);

        int pid;

        size_t shmem_len;
        char *shmem;

        unsigned int *instruction_block_begin;
        unsigned int *instruction_ptr;
        unsigned int *instruction_ptr_end;

        unsigned int *signal_ptr_start, *signal_ptr_end;
        unsigned int *signal_ptr;

        SignalList signals;
        ParameterList parameters;
        VariableMap variableMap;

        std::vector<std::vector<Signal*> > subscribers;
        std::vector<size_t> blockLength;
        std::vector<size_t> iterationNo;
        std::vector<bool> subscribed;
        std::vector<size_t> subscriptionIndex;
        typedef std::map<uint8_t, std::vector<Signal*>, std::greater<uint8_t> >
            SignalWidthMap;
        std::vector<SignalWidthMap> subscriptionMap;

        void post(Instruction, unsigned int param,
                const char* buf = 0, size_t len = 0);
        void post(Instruction);

        // Methods used by the real-time process to interpret inbox
        // instructions from the clients
        void processPollSignal(const struct timespec *time);
        void processSubscribe(bool*);
        void processUnsubscribe(bool*);
        void processSetValue();

        static int localtime(struct timespec *);
};

}
#endif // MAIN_H
