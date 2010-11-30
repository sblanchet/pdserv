/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef SIGNAL
#define SIGNAL

#include "Variable.h"

namespace HRTLab {

class Main;
class Task;
class Session;

class Signal: public Variable {
    public:
        Signal( Main *main,
                const Task *task,
                unsigned int decimation,
                const char *path,
                enum si_datatype_t dtype,
                unsigned int ndims = 1,
                const size_t *dim = 0);

        virtual ~Signal();

        const unsigned int decimation;
        const Task * const task;

        virtual void subscribe(Session *) const = 0;
        virtual void unsubscribe(Session *) const = 0;

    private:
};

}

#endif //SIGNAL
