#ifndef MAIN_H
#define MAIN_H

#include <list>

#include <rtlab/rtlab.h>

namespace HRTLab {

class Signal;
class Parameter;

class Main {
    public:
        Main(int argc, const char *argv[], unsigned int nst);
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

    private:

        const unsigned int nst;

        int pid;

        size_t shmem_len;
        void *shmem;

        std::list<Signal*> signals;
        std::list<Parameter*> parameters;
};

}
#endif // MAIN_H
