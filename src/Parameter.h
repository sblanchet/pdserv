/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef PARAMETER
#define PARAMETER

#include <ctime>
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
                paramupdate_t paramcheck,
                paramupdate_t paramupdate,
                void *priv_data);

        const struct timespec& getMtime() const;

        void setValue(const char *valbuf,
                size_t valbuflen = 0, size_t offset = 0);

    private:

        Main * const main;
        const paramupdate_t paramcheck;
        const paramupdate_t paramupdate;
        void * const priv_data;

        char * const addr;
        struct timespec mtime;

        static int copy(void *dst, const void *src, size_t len, void *);
};

}

#endif //PARAMETER
