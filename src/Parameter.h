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
        Parameter ( Main *main,
                const char *path,
                unsigned int mode,
                enum si_datatype_t dtype,
                unsigned int ndims = 1,
                const size_t *dim = 0);

        virtual ~Parameter();

        const Main * const main;

        const unsigned int mode;

        int setValue(const char *valbuf) const;

        // Reimplemented from HRTLab::Variable
        void getValue(char *buf, struct timespec* t = 0) const;

    protected:
};

}

#endif //PARAMETER
