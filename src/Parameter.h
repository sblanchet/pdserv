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
                Main *main,
                unsigned int index,
                const char *path,
                const char *alias,
                enum si_datatype_t dtype,
                unsigned int ndims,
                const size_t dim[],
                char *addr,
                paramupdate_t paramcheck = copy,
                paramupdate_t paramupdate = copy,
                void *priv_data = 0);

        struct timespec getMtime() const;

        void setValue(const char *valbuf,
                size_t nelem = 0, size_t offset = 0);

    private:

        Main * const main;
        const paramupdate_t paramcheck;
        const paramupdate_t paramupdate;
        void * const priv_data;

        char * const addr;
        struct timespec mtime;

        mutable ost::Semaphore mutex;

        // A default function used when paramcheck or paramupdate are not
        // specified by the user
        static int copy(void *dst, const void *src, size_t len, void *);
};

}

#endif //PARAMETER
