/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef PARAMETER
#define PARAMETER

#include "Variable.h"
#include "rtlab/rtlab.h"

namespace HRTLab {

class Parameter: public Variable {
    public:
        Parameter (
                paramupdate_t paramupdate,
                void *priv_data,
                const char *path,
                const char *alias,
                enum si_datatype_t dtype,
                unsigned int ndims,
                const size_t dim[],
                const void *addr);

        const paramupdate_t paramupdate;
        void * const priv_data;

    private:

};

}

#endif //PARAMETER
