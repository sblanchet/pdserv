/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef LIB_SIGNAL
#define LIB_SIGNAL

#include "../Signal.h"

class Task;
class Main;

class Signal: public HRTLab::Signal {
    public:
        Signal( Main *main,
                const Task *task,
                unsigned int decimation,
                const char *path,
                enum si_datatype_t dtype,
                const void *addr,
                unsigned int ndims = 1,
                const size_t *dim = 0);

        const void * const addr;

    private:
        const Task * const task;

        // Reimplemented from HRTLab::Signal
        const char *getValue(const HRTLab::Session *s) const;
        void getValue(char *, struct timespec *) const;
        void subscribe(HRTLab::Session *) const;
        void unsubscribe(HRTLab::Session *) const;

};

#endif //LIB_SIGNAL
