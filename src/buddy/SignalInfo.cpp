/*****************************************************************************
 *
 *  $Id: Parameter.cpp,v ca2d0581b018 2011/11/18 21:54:07 lerichi $
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

#include <algorithm>
#include <cerrno>

#include "Main.h"
#include "../Debug.h"
#include "Parameter.h"

//////////////////////////////////////////////////////////////////////
template <class T, char dir>
void SignalInfo::copy(void *_dst, const void *_src,
        size_t start, size_t count, size_t , size_t)
{
    const T *src =
        reinterpret_cast<const T*>(_src) + (dir == 'R' ? start : 0);

    std::copy(src, src + count,
            reinterpret_cast<T*>(_dst) + (dir == 'W' ? start : 0));
}

//////////////////////////////////////////////////////////////////////
template <class T, char dir>
void SignalInfo::transpose( void *_dst, const void *_src,
        size_t start, size_t count, size_t nelem, size_t cols)
{
    const T* src = reinterpret_cast<const T*>(_src);
    T* dst = reinterpret_cast<T*>(_dst);
    const size_t rows = nelem / cols;
    size_t n = rows * (start % cols) + (start / cols);

    while (count--) {

        if      (dir == 'W')    dst[n] = *src++;
        else if (dir == 'R')    *dst++ = src[n];

        n += rows;
        if (n >= nelem)
            n -= nelem - 1;
    }
}

//////////////////////////////////////////////////////////////////////
SignalInfo::SignalInfo( const char *model, const struct signal_info *si):
    si(si), model(model)
{
    nelem = si->dim[0] * si->dim[1];
    readFunc = 0;

    if (si->dim[0] == 1 or si->dim[1] == 1) {
        _dim[0] = nelem;
        _dim[1] = 0;
    }
    else {
        std::copy(si->dim, si->dim + 2, _dim);
        if (si->orientation == si_matrix_col_major) {
            switch (PdServ::Variable::dataTypeWidth[dataType()]) {
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
        switch (PdServ::Variable::dataTypeWidth[dataType()]) {
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
    return
        std::string(1,'/').append(model)
        .append(1,'/').append(si->path)
        .append(1,'/').append(si->name);
}

//////////////////////////////////////////////////////////////////////
const size_t *SignalInfo::dim() const
{
    return _dim;
}

//////////////////////////////////////////////////////////////////////
size_t SignalInfo::ndim() const
{
    return (_dim[0] > 0) + (_dim[1] > 0);
}

//////////////////////////////////////////////////////////////////////
PdServ::Variable::Datatype SignalInfo::dataType() const
{
    switch (si->data_type) {
        case si_boolean_T: return PdServ::Variable::boolean_T;
        case si_uint8_T:   return PdServ::Variable::uint8_T;
        case si_sint8_T:   return PdServ::Variable::int8_T;
        case si_uint16_T:  return PdServ::Variable::uint16_T;
        case si_sint16_T:  return PdServ::Variable::int16_T;
        case si_uint32_T:  return PdServ::Variable::uint32_T;
        case si_sint32_T:  return PdServ::Variable::int32_T;
        case si_double_T:  return PdServ::Variable::double_T;
        case si_single_T:  return PdServ::Variable::single_T;
        default:           return PdServ::Variable::double_T;
    }
}

//////////////////////////////////////////////////////////////////////
void SignalInfo::read( void *dst, const void *src,
        size_t start, size_t count) const
{
    readFunc(dst, src, start, count, nelem, _dim[1]);
}

//////////////////////////////////////////////////////////////////////
void SignalInfo::write( void *dst, const void *src,
        size_t start, size_t count) const
{
}
