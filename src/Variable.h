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
                unsigned int index,
                const char *path,
                const char *alias,
                enum si_datatype_t dtype,
                unsigned int ndims,
                const size_t dim[],
                unsigned int decimation,
                const char *addr);

        const unsigned int index;
        const size_t width;
        const size_t memSize;
        const std::string path;
        const std::string alias;
        const enum si_datatype_t dtype;
        const std::vector<size_t> dim;
        const unsigned int decimation;
        const char * const addr;

    private:

        static size_t getDTypeSize(enum si_datatype_t dtype);
        static size_t getMemSize(enum si_datatype_t dtype, 
                unsigned int dims, const size_t dim[]);
};

}
#endif //VARIABLE
