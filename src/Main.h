#ifndef MAIN_H
#define MAIN_H

#include <vector>
#include <map>
#include <string>

#include <rtlab/rtlab.h>

namespace HRTLab {

class Signal;
class Parameter;
class Variable;

template <class T1>
    T1 align(unsigned int n)
    {
        T1 mask = sizeof(T1) - 1;
        return (n + mask) & ~mask;
    }

class Main {
    public:
        enum Instruction {Restart = 1,
            SubscriptionList, Subscribe, Unsubscribe, SetValue,
            NewSubscriberList, SubscriptionData
        };

        Main(int argc, const char *argv[],
                const char *name, const char *version,
                double baserate,
                unsigned int nst, const unsigned int decimation[]);
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
                const void *addr
                );

        int newParameter(
                paramupdate_t paramupdate,
                void *priv_data,
                const char *path,
                const char *alias,
                enum si_datatype_t datatype,
                unsigned int ndims,
                const unsigned int dim[],
                const void *addr
                );

        const std::string name;
        const std::string version;
        const double baserate;
        const unsigned int nst;
        unsigned int * const decimation;

        const std::vector<Signal*>& getSignals() const;
        const std::vector<Parameter*>& getParameters() const;
        const std::map<std::string,Variable*>& getVariableMap() const;

        void subscribe(const std::string& path);

        unsigned int *getSignalPtrStart() const;

    private:

        int pid;

        size_t shmem_len;
        char *shmem;

        unsigned int *instruction_block_begin;
        unsigned int *instruction_ptr;
        unsigned int *instruction_ptr_end;

        unsigned int *signal_ptr_start, *signal_ptr_end;
        unsigned int *signal_ptr;

        std::vector<Signal*> signals;
        std::vector<Parameter*> parameters;
        typedef std::map<std::string,Variable*> VariableMap;
        VariableMap variableMap;

        std::vector<std::vector<Signal*> > subscribers;
        std::vector<size_t> blockLength;
        std::vector<unsigned int> iterationNo;
};

}
#endif // MAIN_H
