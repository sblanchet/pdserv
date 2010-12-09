/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef LIB_SIGNAL
#define LIB_SIGNAL

#include <set>
#include <cc++/thread.h>

#include "../Signal.h"

namespace HRTLab {
    class Session;
}

class Task;
class Main;

class Signal: public HRTLab::Signal {
    public:
        Signal( Main *main,
                const Task *task,
                size_t index,
                unsigned int decimation,
                const char *path,
                enum si_datatype_t dtype,
                const void *addr,
                unsigned int ndims = 1,
                const size_t *dim = 0);

        const size_t index;

        const void * const addr;
        const Task * const task;

        const size_t subscriptionIndex;

        typedef std::set<const HRTLab::Session*> SessionSet;
        mutable SessionSet sessions;

    private:

        static const size_t dataTypeIndex[HRTLab::Variable::maxWidth+1];

        // Reimplemented from HRTLab::Variable
        void getValue(char *, struct timespec *) const;

        // Reimplemented from HRTLab::Signal
        void subscribe(const HRTLab::Session *) const;
        void unsubscribe(const HRTLab::Session *) const;
};

#endif //LIB_SIGNAL
