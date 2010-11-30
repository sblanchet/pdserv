/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef VARIABLE
#define VARIABLE

#include "rtlab/etl_data_info.h"

#include <cstddef>
#include <string>

struct timespec;

namespace HRTLab {

class Variable {
    public:
        Variable( const char *path,
                enum si_datatype_t dtype,
                unsigned int ndims = 1,
                const size_t dim[] = 0);

        virtual ~Variable();

        const std::string path;         /**< Variable path */
        const enum si_datatype_t dtype; /**< Data type */
        const size_t ndims;             /**< Number of dimensions */
        const size_t width;             /**< Width of a single element */
        const size_t nelem;             /**< Number of elements */
        const size_t memSize;           /**< Total memory size */

        std::string alias;              /**< Optional alias */
        std::string unit;               /**< Optional unit */
        std::string comment;            /**< Optional comment */

        const size_t *getDim() const;   /**< Get dimension vector */

        static const size_t maxWidth = 8; /**< Maximum supported data type
                                            width */

        virtual void getValue(char *, struct timespec * = 0) const = 0;

    private:
        size_t * const dim;

        static size_t getDTypeSize(enum si_datatype_t dtype);
        static size_t getNElem( unsigned int dims, const size_t dim[]);
};

}
#endif //VARIABLE
