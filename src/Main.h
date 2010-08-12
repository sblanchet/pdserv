#ifndef MAIN_H
#define MAIN_H

#include <list>
#include <map>
#include <string>

#include <rtlab/rtlab.h>

namespace HRTLab {

class Signal;
class Parameter;
class Variable;

class Main {
    public:
        Main(int argc, const char *argv[],
                const char *name, const char *version,
                double baserate,
                unsigned int nst, const unsigned int decimation[]);
        ~Main();

        void update(int st);
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
        const unsigned int *decimation;

        const std::list<Signal*>& getSignals() const;
        const std::list<Parameter*>& getParameters() const;
        const std::map<std::string,Variable*>& getVariableMap() const;

    private:

        int pid;

        size_t shmem_len;
        void *shmem;

        std::list<Signal*> signals;
        std::list<Parameter*> parameters;
        std::map<std::string,Variable*> variableMap;
};

}
#endif // MAIN_H
