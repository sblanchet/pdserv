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

#include <cstddef>
#include <string>

struct timespec;

namespace PdServ {

class Session;

class Variable {
    public:
        enum Datatype {
            boolean_T,
            uint8_T,  int8_T,
            uint16_T, int16_T,
            uint32_T, int32_T,
            uint64_T, int64_T,
            double_T, single_T
        };

        Variable( const std::string& path,
                Datatype dtype,
                size_t ndims = 1,
                const size_t dim[] = 0);

        virtual ~Variable();

        const std::string path;         /**< Variable path */
        const Datatype dtype;           /**< Data type */
        const size_t ndims;             /**< Number of dimensions */
        const size_t width;             /**< Width of a single element */
        const size_t nelem;             /**< Number of elements */
        const size_t memSize;           /**< Total memory size */

        std::string alias;              /**< Optional alias */
        std::string unit;               /**< Optional unit */
        std::string comment;            /**< Optional comment */

        const size_t *getDim() const;   /**< Get dimension vector */

        static const size_t maxWidth = 8; /**< Maximum supported data type
                                            width in bytes */

        virtual void getValue(const Session*, void *buf,
                struct timespec * = 0) const = 0;

        static const size_t dataTypeWidth[11];
    private:
        size_t * const dim;

        static size_t getNElem( size_t dims, const size_t dim[]);
};

}
#endif //VARIABLE
