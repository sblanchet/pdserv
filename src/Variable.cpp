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

#include "Variable.h"

#include <stdint.h>
#include <sstream>

using namespace PdServ;

//////////////////////////////////////////////////////////////////////
const size_t Variable::dataTypeSize[11] = {
    sizeof(uint8_t),
    sizeof(uint8_t ), sizeof(int8_t),
    sizeof(uint16_t), sizeof(int16_t),
    sizeof(uint32_t), sizeof(int32_t),
    sizeof(uint64_t), sizeof(int64_t),
    sizeof(double  ), sizeof(float)
};

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
namespace PdServ {

    /////////////////////////////////////////////////////////////////////////
    template <class T>
        void Variable::print(std::ostream &os,
                const void* data, size_t n)
        {
            os << reinterpret_cast<const T*>(data)[n];
        }

    /////////////////////////////////////////////////////////////////////////
    template<>
        void Variable::print<int8_t>(std::ostream& os,
                const void *data, size_t n)
        {
            os << (int)reinterpret_cast<const int8_t*>(data)[n];
        }

    /////////////////////////////////////////////////////////////////////////
    template<>
        void Variable::print<uint8_t>(std::ostream& os,
                const void *data, size_t n)
        {
            os << (unsigned int)reinterpret_cast<const uint8_t*>(data)[n];
        }
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
Variable::Variable(
                const std::string& path,
                const Datatype& dtype,
                size_t ndims,
                const size_t *dim):
    path(path),

    dtype(dtype),
    elemSize(dataTypeSize[dtype]),

    nelem(getNElem(ndims, dim)),
    memSize(nelem * elemSize),

    ndims(dim ? ndims : 1),
    dim(makeDimVector(ndims, dim))
{
    switch (dtype) {
        case boolean_T: printFunc = print<bool>;      break;
        case uint8_T:   printFunc = print<uint8_t>;   break;
        case int8_T:    printFunc = print<int8_t>;    break;
        case uint16_T:  printFunc = print<uint16_t>;  break;
        case int16_T:   printFunc = print<int16_t>;   break;
        case uint32_T:  printFunc = print<uint32_t>;  break;
        case int32_T:   printFunc = print<int32_t>;   break;
        case uint64_T:  printFunc = print<uint64_t>;  break;
        case int64_T:   printFunc = print<int64_t>;   break;
        case double_T:  printFunc = print<double>;    break;
        case single_T:  printFunc = print<float>;     break;
    }
}

//////////////////////////////////////////////////////////////////////
Variable::~Variable()
{
    delete[] dim;
}

//////////////////////////////////////////////////////////////////////
void Variable::print(std::ostream& os,
        const void *data, size_t nelem, char delim) const
{
    if (!nelem)
        nelem = this->nelem;

    for (size_t i = 0; i < nelem; i++) {
        if (i)
            os << delim;
        printFunc(os, data, i);
    }
}

//////////////////////////////////////////////////////////////////////
size_t Variable::getNElem( size_t ndims, const size_t dim[])
{
    if (dim) {
        size_t n = 1;

        for (size_t i = 0; i < ndims; i++)
            n *= dim[i];

        return n;
    }
    else
        return ndims;
}

//////////////////////////////////////////////////////////////////////
const size_t *Variable::makeDimVector( size_t ndims, const size_t _dim[])
{
    size_t *dim = new size_t[_dim ? ndims : 1];

    if (_dim)
        std::copy(_dim, _dim + ndims, dim);
    else
        *dim = ndims;

    return dim;
}

