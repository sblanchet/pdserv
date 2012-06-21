/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdserv library.
 *
 *  The pdserv library is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or (at
 *  your option) any later version.
 *
 *  The pdserv library is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with the pdserv library. If not, see <http://www.gnu.org/licenses/>.
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
                const Datatype& dtype,
                size_t ndims = 1,
                const size_t dim[] = 0);

        virtual ~Variable();

        void print(std::ostream& os,
                const void *data, size_t nelem = 0, char delim = ',') const;

        const std::string path;         /**< Variable path */
        const Datatype& dtype;          /**< Data type */
        const size_t elemSize;          /**< Bytes for a single element */

        const size_t nelem;             /**< Number of elements */
        const size_t memSize;           /**< Total memory size */

        const size_t ndims;             /**< Number of dimensions */
        size_t const* const dim;        /**< The dimension vector */

        std::string alias;              /**< Optional alias */
        std::string unit;               /**< Optional unit */
        std::string comment;            /**< Optional comment */

        static const size_t maxWidth = 8; /**< Maximum supported data type
                                            size in bytes */

        // This method directly copies the variable's value into buf. This
        // method has to be used when a variable's value is valid only within
        // the method's calling context
        virtual void getValue(
                const Session*,         /**< Calling session */
                void *buf,              /**< Buffer where data is copied to */
                struct ::timespec * = 0   /**< Optional timespec stamp */
                ) const = 0;

        static const size_t dataTypeSize[11];
    private:

        typedef void (*PrintFunc)(std::ostream&,
                const void *, size_t);
        PrintFunc printFunc;

        template <class T>
            static void print(std::ostream& os,
                    const void *data, size_t n);

        static size_t getNElem( size_t dims, const size_t dim[]);
        static const size_t *makeDimVector( size_t len, const size_t dim[]);
};

}
#endif //VARIABLE
