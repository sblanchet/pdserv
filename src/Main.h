#ifndef MAIN_H
#define MAIN_H

#include <list>

#include <rtlab/etl_data_info.h>

namespace HRTLab {

class Signal;

class Main {
    public:
        Main(int argc, const char *argv[], unsigned int nst);
        ~Main();

        void update(int st);

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

    private:
        const unsigned int nst;

        std::list<Signal*> signals;
};

}
#endif // MAIN_H
