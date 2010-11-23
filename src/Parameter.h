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
                unsigned int index,
                const char *path,
                unsigned int mode,
                enum si_datatype_t dtype,
                unsigned int ndims,
                const size_t dim[],
                char *addr,
                paramupdate_t paramcopy = copy,
                void *priv_data = 0);

        // Called in real time context
        int setValue(unsigned int tid, const char *valbuf) const;

        const unsigned int mode;

    private:
        const paramupdate_t paramcopy;
        void * const priv_data;
        char * const addr;


        // A default function used when paramcheck or paramupdate are not
        // specified by the user
        static int copy(unsigned int tid,
                void *dst, const void *src, size_t len, void *);
};

}

#endif //PARAMETER
