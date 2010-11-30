/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef LIB_PARAMETER
#define LIB_PARAMETER

#include <ctime>
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

        void * const addr;      // Pointer to the real address

        char *shmemAddr;
        struct timespec mtime;
        paramtrigger_t trigger;
        void *priv_data;

    private:
        const Main * const main;

        // Reimplemented from HRTLab::Parameter
        int setValue(const char *valbuf) const;
        void getValue(char *, struct timespec *t) const;

        // A default function used when paramcheck or paramupdate are not
        // specified by the user
        static int copy(unsigned int tid, char checkOnly,
                void *dst, const void *src, size_t len, void *priv_data);
};

#endif //LIB_PARAMETER
