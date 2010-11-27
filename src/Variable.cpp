/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "config.h"

#include "Variable.h"

#include <stdint.h>

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace HRTLab;

//////////////////////////////////////////////////////////////////////
Variable::Variable(
                const char *path,
                enum si_datatype_t dtype,
                unsigned int ndims,
                const size_t *dim):
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
size_t Variable::getNElem( unsigned int ndims, const size_t dim[])
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
