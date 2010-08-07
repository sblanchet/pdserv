/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef VARIABLE
#define VARIABLE

#include "rtlab/etl_data_info.h"

#include <cstddef>
#include <string>
#include <vector>

namespace HRTLab {

class Variable {
    public:
        Variable(
                const char *path,
                const char *alias,
                enum si_datatype_t dtype,
                unsigned int ndims,
                const size_t dim[],
                const void *addr);

        const size_t width;
        const size_t memSize;
        const std::string path;
        const std::string alias;
        const enum si_datatype_t dtype;
        const std::vector<size_t> dim;
        const void * const addr;

    private:

        static size_t getDTypeSize(enum si_datatype_t dtype);
        static size_t getMemSize(enum si_datatype_t dtype, 
                unsigned int dims, const size_t dim[]);
};

}
#endif //VARIABLE
