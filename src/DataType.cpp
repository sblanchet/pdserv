/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright (C) 2012 Richard Hacker (lerichi at gmx dot net)
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

#include "DataType.h"
#include <stdint.h>
#include <ostream>
#include <functional>   // std::multiplies
#include <numeric>      // std::accumulate


using namespace PdServ;

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
namespace {
    template <class T>
        class PrimaryType: public DataType {
            public:
                PrimaryType(): DataType(sizeof(T)) {
                }

            private:
                std::ostream& print(std::ostream& os,
                        const char *data, size_t n) const {
                    const T* val = reinterpret_cast<const T*>(data);
                    char delim = '\0';

                    while (n--) {
                        if (delim)
                            os << delim;
                        delim = ',';

                        os << *val++;
                    }
                    return os;
                }
        };

    template<>
        std::ostream& PrimaryType<int8_t>::print(std::ostream& os,
                const char *data, size_t n) const {
            const int8_t* val = reinterpret_cast<const int8_t*>(data);
            char delim = '\0';

            while (n--) {
                if (delim)
                    os << delim;
                delim = ',';

                os << (int)*val++;
            }
            return os;
        }

    template<>
        std::ostream& PrimaryType<uint8_t>::print(std::ostream& os,
                const char *data, size_t n) const {
            const uint8_t* val = reinterpret_cast<const uint8_t*>(data);
            char delim = '\0';

            while (n--) {
                if (delim)
                    os << delim;
                delim = ',';

                os << (unsigned)*val++;
            }
            return os;
        }
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
const DataType& DataType::boolean = PrimaryType<    bool>();
const DataType& DataType::  uint8 = PrimaryType< uint8_t>();
const DataType& DataType::   int8 = PrimaryType<  int8_t>();
const DataType& DataType:: uint16 = PrimaryType<uint16_t>();
const DataType& DataType::  int16 = PrimaryType< int16_t>();
const DataType& DataType:: uint32 = PrimaryType<uint32_t>();
const DataType& DataType::  int32 = PrimaryType< int32_t>();
const DataType& DataType:: uint64 = PrimaryType<uint64_t>();
const DataType& DataType::  int64 = PrimaryType< int64_t>();
const DataType& DataType::float64 = PrimaryType<  double>();
const DataType& DataType::float32 = PrimaryType<   float>();

//////////////////////////////////////////////////////////////////////
const DataType* primaryTypes[11] = {
    &DataType::boolean,
    &DataType::uint8,   &DataType:: int8,
    &DataType::uint16,  &DataType::int16,
    &DataType::uint32,  &DataType::int32,
    &DataType::uint64,  &DataType::int64,
    &DataType::float64, &DataType::float32
};

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// namespace PdServ {
//     std::ostream& operator<<(std::ostream& os,
//             const DataType::FieldList& fieldList)
//     {
//         for (DataType::FieldList::const_iterator it = fieldList.begin();
//                 it != fieldList.end(); ++it) {
//             (*it)->type.printType(name, );
//             //const DataType::Field* f = *it;
//             //if (f->name.empty()) {
//             //}
//             //else {
//             //}
//         }
//         return os;
//     }
// }

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
DataType::Field::Field (const std::string& name, const DataType& type,
        size_t offset, size_t ndims, const size_t *dims):
    name(name), type(type), offset(offset), dims(dims, dims + ndims),
    nelem(std::accumulate(dims, dims + ndims,
                1U, std::multiplies<size_t>()))
{
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
DataType::DataType (const std::string& name, size_t size):
    name(name), size(size)
{
}

//////////////////////////////////////////////////////////////////////
DataType::DataType (const DataType& other):
    name(other.name), size(other.size)
{
}

//////////////////////////////////////////////////////////////////////
DataType::DataType (size_t size): size(size)
{
}

//////////////////////////////////////////////////////////////////////
DataType::~DataType ()
{
    for (FieldList::const_iterator it = fieldList.begin();
            it != fieldList.end(); ++it)
        delete *it;
}

//////////////////////////////////////////////////////////////////////
std::ostream& DataType::print(std::ostream& os,
        const char *data, size_t n) const
{
    char delim = '\0';

    while (n--) {
        for (FieldList::const_iterator it = fieldList.begin();
                it != fieldList.end(); ++it) {
            if (delim)
                os << delim;
            delim = ';';
            (*it)->type.print(os, data + (*it)->offset, (*it)->nelem);
        }
        data += size;
    }

    return os;
}

//////////////////////////////////////////////////////////////////////
void DataType::addField (const std::string& name, const DataType& type,
        size_t offset, size_t ndims, const size_t* dims)
{
    fieldList.push_back(new Field(name, type, offset, dims ? ndims : 1,
                dims ? dims : &ndims));
}

//////////////////////////////////////////////////////////////////////
const DataType::FieldList& DataType::getFieldList () const
{
    return fieldList;
}
