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
                enum si_datatype_t dtype,
                const void *addr,
                unsigned int ndims = 1,
                const size_t dim[] = 0);

        ~Variable();

        const unsigned int index;
        const std::string path;

        void setAlias(const char *);
        void setUnit(const char *);
        void setComment(const char *);

        const std::string& getAlias() const;
        const std::string& getUnit() const;
        const std::string& getComment() const;

        const enum si_datatype_t dtype;
        const size_t ndims;
        const size_t width;
        const size_t nelem;
        const size_t memSize;
        const void * const addr;

        const size_t *getDim() const;

        static const size_t maxWidth = 8;

    private:
        size_t * const dim;

        std::string alias;
        std::string unit;
        std::string comment;

        static size_t getDTypeSize(enum si_datatype_t dtype);
        static size_t getNElem( unsigned int dims, const size_t dim[]);

};

}
#endif //VARIABLE
