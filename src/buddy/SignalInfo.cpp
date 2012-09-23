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

#include "config.h"

#include <algorithm>
#include <sstream>

#include "Main.h"
#include "../Debug.h"
#include "SignalInfo.h"

#ifdef TRADITIONAL
    static const bool traditional = 1;
#else
    static const bool traditional = 0;
#endif

//////////////////////////////////////////////////////////////////////
template <typename T, char dir>
void SignalInfo::copy(void *_dst, const void *_src,
        size_t *offset, size_t *count, size_t , size_t)
{
    const T *src = reinterpret_cast<const T*>(_src);

    std::copy(src, src + *count / sizeof(T),
            reinterpret_cast<T*>(_dst) + *offset / sizeof(T));
}

//////////////////////////////////////////////////////////////////////
template <typename T, char dir>
void SignalInfo::transpose( void *_dst, const void *_src,
        size_t *offset, size_t *count, size_t nelem, size_t cols)
{
    const T* src = reinterpret_cast<const T*>(_src);
    T* dst = reinterpret_cast<T*>(_dst);

    const size_t rows = nelem / cols;
    size_t i = (rows * (*offset % cols) + (*offset / cols)) / sizeof(T);
    size_t n = *count / sizeof(T);

    if (dir == 'W') {
        *offset = i * sizeof(T);
        *count  = ((n - 1) * rows + 1) * sizeof(T);
    }

    nelem--;
    while (n--) {

        if      (dir == 'W')    dst[i] = *src++;
        else if (dir == 'R')    *dst++ = src[i];

        i += cols;
        if (i > nelem)
            i -= nelem;
    }

}

//////////////////////////////////////////////////////////////////////
SignalInfo::SignalInfo( const char *model, const struct signal_info *si):
    si(si), model(model)
{
    size_t type_size = dataType().size;

    nelem = si->dim[0] * si->dim[1];
    memsize = type_size * nelem;

    readFunc = 0;

    if (si->dim[0] == 1 or si->dim[1] == 1) {
        dim[0] = nelem;
        dim[1] = 0;
    }
    else {
        std::copy(si->dim, si->dim + 2, dim);
        if (si->orientation == si_matrix_col_major) {
            switch (type_size) {
                case 1:
                    readFunc  = transpose<uint8_t,'R'>;
                    writeFunc = transpose<uint8_t,'W'>;
                    break;
                case 2:
                    readFunc  = transpose<uint16_t,'R'>;
                    writeFunc = transpose<uint16_t,'W'>;
                    break;
                case 4:
                    readFunc  = transpose<uint32_t,'R'>;
                    writeFunc = transpose<uint32_t,'W'>;
                    break;
                case 8:
                    readFunc  = transpose<uint64_t,'R'>;
                    writeFunc = transpose<uint64_t,'W'>;
                    break;
            }
        }
    }

    if (!readFunc) {
        switch (type_size) {
            case 1:
                readFunc  = copy<uint8_t,'R'>;
                writeFunc = copy<uint8_t,'W'>;
                break;
            case 2:
                readFunc  = copy<uint16_t,'R'>;
                writeFunc = copy<uint16_t,'W'>;
                break;
            case 4:
                readFunc  = copy<uint32_t,'R'>;
                writeFunc = copy<uint32_t,'W'>;
                break;
            case 8:
                readFunc  = copy<uint64_t,'R'>;
                writeFunc = copy<uint64_t,'W'>;
                break;
        }
    }
}

//////////////////////////////////////////////////////////////////////
std::string SignalInfo::path() const
{
    std::ostringstream p;

    if (traditional)
        p << '/' << model;

    p << '/' << si->path << '/' << si->name;

#ifdef HAVE_SIMULINK_PORT
    if (!traditional and si->port) {
        p << '/';
        if (*si->alias)
            p << si->alias;
        else
            p << si->port-1;
    }
#endif

    return p.str();
}

//////////////////////////////////////////////////////////////////////
const size_t *SignalInfo::getDim() const
{
    return dim;
}

//////////////////////////////////////////////////////////////////////
size_t SignalInfo::ndim() const
{
    return (dim[0] > 0) + (dim[1] > 0);
}

//////////////////////////////////////////////////////////////////////
const PdServ::DataType& SignalInfo::dataType() const
{
    switch (si->data_type) {
        case si_boolean_T: return PdServ::DataType::boolean;
        case si_uint8_T:   return PdServ::DataType::uint8;
        case si_sint8_T:   return PdServ::DataType::int8;
        case si_uint16_T:  return PdServ::DataType::uint16;
        case si_sint16_T:  return PdServ::DataType::int16;
        case si_uint32_T:  return PdServ::DataType::uint32;
        case si_sint32_T:  return PdServ::DataType::int32;
        case si_double_T:  return PdServ::DataType::float64;
        case si_single_T:  return PdServ::DataType::float32;
        default:           return PdServ::DataType::float64;
    }
}

//////////////////////////////////////////////////////////////////////
void SignalInfo::read( void *dst, const void *src) const
{
    size_t i = 0;
    size_t n = memsize;

    readFunc(dst, src, &i, &n, nelem, dim[1]);
}

//////////////////////////////////////////////////////////////////////
void SignalInfo::write( void *dst, const void *src,
        size_t *offset, size_t *count) const
{
    writeFunc(dst, src, offset, count, nelem, dim[1]);
}

