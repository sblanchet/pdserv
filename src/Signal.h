/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef SIGNAL
#define SIGNAL

#include "Variable.h"

namespace HRTLab {

class Signal: public Variable {
    public:
        Signal( unsigned int index,
                unsigned int tid,
                unsigned int decimation,
                const char *path,
                enum si_datatype_t dtype,
                const void *addr,
                unsigned int ndims = 1,
                const size_t *dim = 0);

        const unsigned int tid;
        const unsigned int decimation;

//        void log(
//                /** Signal decimation with which this signal will be logged.
//                 * Decimation is with respect to the signal sample time,
//                 * not the task sample time */
//                unsigned int decimation
//                ) const;
//
//        void cancelLog() const;

    private:
};

}

#endif //SIGNAL
