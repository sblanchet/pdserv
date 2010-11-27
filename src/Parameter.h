/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef PARAMETER
#define PARAMETER

#include <ctime>
#include <cc++/thread.h>
#include "Variable.h"
#include "rtlab/rtlab.h"

namespace HRTLab {

class Main;

class Parameter: public Variable {
    public:
        Parameter ( unsigned int index,
                const char *path,
                unsigned int mode,
                enum si_datatype_t dtype,
                void *addr,
                unsigned int ndims = 1,
                const size_t *dim = 0);

        const unsigned int index;
        void * const addr;

        // Called in real time context
        int hrtSsetValue(unsigned int tid, const char *valbuf) const;

        void setTrigger(paramtrigger_t trigger, void *priv_data);
        struct timespec getMTime() const;

        const unsigned int mode;

    private:
        paramtrigger_t trigger;
        void *priv_data;



        // A default function used when paramcheck or paramupdate are not
        // specified by the user
        static int copy(unsigned int tid,
                void *dst, const void *src, size_t len, void *);
};

}

#endif //PARAMETER
