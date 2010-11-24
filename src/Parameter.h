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
        Parameter (
                const char *path,
                unsigned int mode,
                enum si_datatype_t dtype,
                void *addr,
                unsigned int ndims = 1,
                const size_t *dim = 0);

        // Called in real time context
        int setValue(unsigned int tid, const char *valbuf) const;

        void setTrigger(paramtrigger_t trigger, void *priv_data);

        const unsigned int mode;

    private:
        paramtrigger_t trigger;
        void *priv_data;

        void * const addr;


        // A default function used when paramcheck or paramupdate are not
        // specified by the user
        static int copy(unsigned int tid,
                void *dst, const void *src, size_t len, void *);
};

}

#endif //PARAMETER
