/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef PARAMETER
#define PARAMETER

#include "Variable.h"

struct timespec;

namespace HRTLab {

class Main;

class Parameter: public Variable {
    public:
        Parameter (
                Main *main,
                const char *path,
                unsigned int mode,
                enum si_datatype_t dtype,
                unsigned int ndims = 1,
                const size_t *dim = 0);

        virtual ~Parameter();

        const unsigned int mode;

        virtual int setValue(const char *valbuf) = 0;

    private:
        Main * const main;
};

}

#endif //PARAMETER
