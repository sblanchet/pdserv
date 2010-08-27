/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef PARAMETER
#define PARAMETER

#include <ctime>
#include "Variable.h"
#include "rtlab/rtlab.h"

namespace HRTLab {

class Parameter: public Variable {
    public:
        Parameter (
                unsigned int index,
                const char *path,
                const char *alias,
                enum si_datatype_t dtype,
                unsigned int ndims,
                const size_t dim[],
                char *addr,
                paramupdate_t paramcheck,
                paramupdate_t paramupdate,
                void *priv_data);

        const paramupdate_t paramcheck;
        const paramupdate_t paramupdate;
        void * const priv_data;

        const struct timespec& getMtime() const;

    private:

        char * const addr;
        struct timespec mtime;

};

}

#endif //PARAMETER
