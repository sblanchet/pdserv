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
const size_t Variable::dataTypeSize[11] = {
    sizeof(uint8_t),
    sizeof(uint8_t ), sizeof(int8_t),
    sizeof(uint16_t), sizeof(int16_t),
    sizeof(uint32_t), sizeof(int32_t),
    sizeof(uint64_t), sizeof(int64_t),
    sizeof(double  ), sizeof(float)
};

//////////////////////////////////////////////////////////////////////
Variable::Variable(
                const std::string& path,
                Datatype dtype,
                size_t ndims,
                const size_t *dim):
    path(path),

    dtype(dtype),
    elemSize(dataTypeSize[dtype]),

    nelem(getNElem(ndims, dim)),
    memSize(nelem * elemSize),

    ndims(dim ? ndims : 1),
    dim(makeDimVector(this->ndims, dim))
{
}

//////////////////////////////////////////////////////////////////////
Variable::~Variable()
{
    delete[] dim;
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
    else {
        return ndims;
    }
}

//////////////////////////////////////////////////////////////////////
const size_t *Variable::makeDimVector( size_t ndims, const size_t _dim[])
{
    size_t *dim = new size_t[ndims];

    if (_dim)
        std::copy(_dim, _dim+ndims, dim);
    else
        *dim = ndims;

    return dim;
}
