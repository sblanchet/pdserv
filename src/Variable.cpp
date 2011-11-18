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

#include "Variable.h"

#include <stdint.h>

using namespace PdServ;

//////////////////////////////////////////////////////////////////////
Variable::Variable(
                const char *path,
                enum si_datatype_t dtype,
                unsigned int ndims,
                const unsigned int *dim):
    path(path), dtype(dtype),
    ndims(dim ? ndims : 1),
    width(getDTypeSize(dtype)),
    nelem(getNElem(ndims, dim)),
    memSize(nelem * width),
    dim(new size_t[ndims])
{
    if (dim)
        std::copy(dim, dim+ndims, this->dim);
    else
        *this->dim = ndims;
}

//////////////////////////////////////////////////////////////////////
Variable::~Variable()
{
    delete[] dim;
}

//////////////////////////////////////////////////////////////////////
const size_t *Variable::getDim() const
{
    return dim;
}

//////////////////////////////////////////////////////////////////////
size_t Variable::getDTypeSize(enum si_datatype_t dtype)
{
    switch (dtype) {
        case si_boolean_T:
            return sizeof(uint8_t);
            break;
        case si_uint8_T:
            return sizeof(uint8_t);
            break;
        case si_sint8_T:
            return sizeof(int8_t);
            break;
        case si_uint16_T:
            return sizeof(uint16_t);
            break;
        case si_sint16_T:
            return sizeof(int16_t);
            break;
        case si_uint32_T:
            return sizeof(uint32_t);
            break;
        case si_sint32_T:
            return sizeof(int32_t);
            break;
        case si_uint64_T:
            return sizeof(uint64_t);
            break;
        case si_sint64_T:
            return sizeof(int64_t);
            break;
        case si_double_T:
            return sizeof(double);
            break;
        case si_single_T:
            return sizeof(float);
            break;
        default:
            return 0;
    }
}

//////////////////////////////////////////////////////////////////////
size_t Variable::getNElem( unsigned int ndims, const unsigned int dim[])
{
    if (dim) {
        size_t n = 1;

        for (size_t i = 0; i < ndims; i++)
            n *= dim[i];

        return n;
    }
    else {
        return ndims;
    }
}
