/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef LIB_PARAMETER
#define LIB_PARAMETER

#include <ctime>
#include <cc++/thread.h>
#include "../Parameter.h"
#include "rtlab/rtlab.h"

class Main;

class Parameter: public HRTLab::Parameter {
    public:
        Parameter ( Main *main,
                const char *path,
                unsigned int mode,
                enum si_datatype_t dtype,
                void *addr,
                unsigned int ndims = 1,
                const size_t *dim = 0);

        ~Parameter();

        const size_t index;

        char * const addr;      // Pointer to the real address

        paramtrigger_t trigger;
        void *priv_data;

        void copyValue(const char *valbuf, const struct timespec& t);

    private:

        mutable ost::Semaphore mutex;
        struct timespec mtime;
        char *valueBuf;

        // A default function used when paramcheck or paramupdate are not
        // specified by the user
        static int copy(unsigned int tid, char checkOnly,
                void *dst, const void *src, size_t len, void *priv_data);

        // Reimplemented from HRTLab::Variable
        void getValue(char *buf, struct timespec* t = 0) const;
};

#endif //LIB_PARAMETER
