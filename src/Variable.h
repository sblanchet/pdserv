/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdserv package.
 *
 *  pdserv is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pdserv is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pdserv. See COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#ifndef VARIABLE
#define VARIABLE

#include "pdserv/etl_data_info.h"

#include <cstddef>
#include <string>

struct timespec;

namespace PdServ {

class Variable {
    public:
        Variable( const char *path,
                enum si_datatype_t dtype,
                unsigned int ndims = 1,
                const unsigned int dim[] = 0);

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
        static size_t getNElem( unsigned int dims, const unsigned int dim[]);
};

}
#endif //VARIABLE
