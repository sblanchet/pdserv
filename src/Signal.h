/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef SIGNAL_H
#define SIGNAL_H

#include "Variable.h"

namespace HRTLab {

class Main;
class Task;
class Session;

class Signal: public Variable {
    public:
        Signal( Task *task,
                unsigned int decimation,
                const char *path,
                enum si_datatype_t dtype,
                unsigned int ndims = 1,
                const size_t *dim = 0);

        virtual ~Signal();

        const unsigned int decimation;
        const Task * const task;

        virtual void subscribe(const Session *) const = 0;
        virtual void unsubscribe(const Session *) const = 0;

        // Reimplemented from HRTLab::Variable
        void getValue(char *, struct timespec * = 0) const;

    private:

};

}

#endif //SIGNAL_H
