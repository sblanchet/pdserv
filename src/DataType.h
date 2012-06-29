/*****************************************************************************
 *
 *  $Id$
 *
 *  copyright (C) 2012 Richard Hacker (lerichi at gmx dot net)
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

#ifndef DATATYPE_H
#define DATATYPE_H

#include "Debug.h"

#include <list>
#include <stdint.h>
#include <vector>
#include <string>
#include <ostream>

namespace PdServ {

class DataType {
    public:
        struct DimType: public std::vector<size_t> {
            DimType(size_t ndims, const size_t *dim);
            const size_t nelem;
        };

        struct Field {
            Field(const std::string& name, const DataType& type,
                    size_t offset, size_t ndims, const size_t *dims);
            const std::string name;
            const DataType& type;
            const size_t offset;
            const DimType dim;
        };

        enum Primary {
            compound_T = 0,
            boolean_T,
            uint8_T,  int8_T,
            uint16_T, int16_T,
            uint32_T, int32_T,
            uint64_T, int64_T,
            double_T, single_T
        };

        DataType( const std::string& name, size_t size);
        virtual ~DataType();

        const std::string name;
        const size_t size;
        virtual Primary primary() const;
        static const size_t maxWidth = 8; /**< Maximum supported data type
                                            size in bytes */

        virtual size_t align() const;

        void addField(const std::string& name,
                const DataType& type,
                size_t offset,
                size_t ndims = 1,
                const size_t* dims = 0);

        struct FieldList: public std::list<const Field*> {
            friend std::ostream& operator<<(std::ostream& os,
                    const FieldList& fieldList);
        };
        const FieldList& getFieldList() const;

        bool operator==(const DataType& other) const;
        void (* const setValue)(char *&, double);

        static const DataType& boolean;
        static const DataType&   uint8;
        static const DataType&    int8;
        static const DataType&  uint16;
        static const DataType&   int16;
        static const DataType&  uint32;
        static const DataType&   int32;
        static const DataType&  uint64;
        static const DataType&   int64;
        static const DataType& float64;
        static const DataType& float32;

        virtual std::ostream& print(std::ostream& os,
                const char *data, size_t n) const;

        struct Printer {
            Printer(const DataType* dt, const char *addr, size_t size):
                type(dt), addr(addr), size(size) {
                }

            friend std::ostream& operator<<(std::ostream& os,
                    const Printer& obj) {
                return obj.type->print(os, obj.addr, obj.size);
            }

            const DataType* const type;
            const char * const addr;
            const size_t size;
        };

        Printer operator()(const void *addr, size_t n = 1) const {
            return Printer(this, reinterpret_cast<const char*>(addr), n);
        }

    protected:
        DataType(const DataType& other);
        explicit DataType(size_t size, void (*)(char *&dst, double src));

    private:

        FieldList fieldList;
};

}
#endif //DATATYPE_H
