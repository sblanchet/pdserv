/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "Variable.h"
#include <stdint.h>

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

using namespace HRTLab;

//////////////////////////////////////////////////////////////////////
Variable::Variable(
                unsigned int index,
                const char *path,
                const char *alias,
                enum si_datatype_t dtype,
                unsigned int ndims,
                const size_t dim[],
                unsigned int decimation,
                const char *addr):
    index(index),
    path(path), alias(alias), dtype(dtype),
    ndims(ndims),
    width(getDTypeSize(dtype)),
    nelem(getNElem(ndims, dim)),
    memSize(nelem * width),
    decimation(decimation), addr(addr),
    dim(new size_t[nelem])
{
    std::copy(dim, dim+ndims, this->dim);
    cout << __func__ << endl;
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
