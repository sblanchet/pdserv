/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef LIB_SIGNAL
#define LIB_SIGNAL

#include "../Signal.h"

class Task;

class Signal: public HRTLab::Signal {
    public:
        Signal( Task *task,
                unsigned int decimation,
                const char *path,
                enum si_datatype_t dtype,
                const void *addr,
                unsigned int ndims = 1,
                const size_t *dim = 0);

        const void * const addr;

    private:
        Task * const task;

        // Reimplemented from HRTLab::Signal
        const char *getValue(const HRTLab::Session *s) const;
        void subscribe(HRTLab::Session *) const;
        void unsubscribe(HRTLab::Session *) const;
        void poll(char *buf) const;

};

#endif //LIB_SIGNAL
