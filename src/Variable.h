/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef VARIABLE
#define VARIABLE

#include "rtlab/etl_data_info.h"

#include <cstddef>
#include <string>

namespace HRTLab {

class Variable {
    public:
        Variable(
                unsigned int index,
                const char *path,
                const char *alias,
                enum si_datatype_t dtype,
                unsigned int ndims,
                const size_t dim[],
                unsigned int decimation,
                const char *addr);

        ~Variable();

        const unsigned int index;
        const std::string path;
        const std::string alias;
        const std::string unit;
        const std::string comment;
        const enum si_datatype_t dtype;
        const size_t ndims;
        const size_t width;
        const size_t nelem;
        const size_t memSize;
        const unsigned int decimation;
        const char * const addr;

        const size_t *getDim() const;

        static const size_t maxWidth = 8;

    private:
        size_t * const dim;

        static size_t getDTypeSize(enum si_datatype_t dtype);
        static size_t getNElem( unsigned int dims, const size_t dim[]);

};

}
#endif //VARIABLE
