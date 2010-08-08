/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef SIGNAL
#define SIGNAL

#include "Variable.h"

namespace HRTLab {

class Signal: public Variable {
    public:
        Signal( unsigned int tid,
                unsigned int decimation,
                const char *path,
                const char *alias,
                enum si_datatype_t dtype,
                unsigned int ndims,
                const size_t *dim,
                const void *addr);

        const unsigned int tid;

        /** Decimation with respect to the task sample time with which
         * the signal is calculated. */
        const int signalDecimation;

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