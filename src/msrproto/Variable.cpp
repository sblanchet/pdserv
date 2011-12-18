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
#include "XmlElement.h"
#include "Directory.h"
#include "../Signal.h"
#include "../Parameter.h"

#include <cstdio>
#include <stdint.h>
#include <sstream>

using namespace MsrProto;

namespace MsrProto {

    /////////////////////////////////////////////////////////////////////////
    template <class T>
        void Variable::print(std::ostream &os,
                const void* data, size_t n, unsigned int)
        {
            os << reinterpret_cast<const T*>(data)[n];
        }

    /////////////////////////////////////////////////////////////////////////
    template<>
        void Variable::print<int8_t>(std::ostream& os,
                const void *data, size_t n, unsigned int)
        {
            os << (int)reinterpret_cast<const int8_t*>(data)[n];
        }

    /////////////////////////////////////////////////////////////////////////
    template<>
        void Variable::print<uint8_t>(std::ostream& os,
                const void *data, size_t n, unsigned int)
        {
            os << (unsigned int)reinterpret_cast<const uint8_t*>(data)[n];
        }

    /////////////////////////////////////////////////////////////////////////
    template<>
        void Variable::print<double>(std::ostream& os,
                const void *data, size_t n, unsigned int precision)
        {
            char buf[precision + 10];

            snprintf(buf, sizeof(buf), "%.*g", precision,
                    reinterpret_cast<const double*>(data)[n]);

            os << buf;
        }

    /////////////////////////////////////////////////////////////////////////
    template<>
        void Variable::print<float>(std::ostream& os,
                const void *data, size_t n, unsigned int precision)
        {
            char buf[precision + 10];

            snprintf(buf, sizeof(buf), "%.*g", precision,
                    reinterpret_cast<const float*>(data)[n]);

            os << buf;
        }
}

/////////////////////////////////////////////////////////////////////////////
Variable::Variable( const PdServ::Variable *v, unsigned int variableIndex,
        unsigned int elementIndex):
    elementIndex(elementIndex != ~0U ? elementIndex : 0),
    variable(v),
    variableIndex(variableIndex),
    nelem(elementIndex == ~0U ? v->nelem : 1), memSize(nelem * v->width)
//    bufferOffset(elementIndex == ~0U ? 0 : elementIndex * v->width)
{
    switch (v->dtype) {
        case PdServ::Variable::boolean_T: printFunc = print<bool>;      break;
        case PdServ::Variable::uint8_T:   printFunc = print<uint8_t>;   break;
        case PdServ::Variable::int8_T:    printFunc = print<int8_t>;    break;
        case PdServ::Variable::uint16_T:  printFunc = print<uint16_t>;  break;
        case PdServ::Variable::int16_T:   printFunc = print<int16_t>;   break;
        case PdServ::Variable::uint32_T:  printFunc = print<uint32_t>;  break;
        case PdServ::Variable::int32_T:   printFunc = print<int32_t>;   break;
        case PdServ::Variable::uint64_T:  printFunc = print<uint64_t>;  break;
        case PdServ::Variable::int64_T:   printFunc = print<int64_t>;   break;
        case PdServ::Variable::double_T:  printFunc = print<double>;    break;
        case PdServ::Variable::single_T:  printFunc = print<float>;     break;
    }
}

/////////////////////////////////////////////////////////////////////////////
Variable::~Variable()
{
}

/////////////////////////////////////////////////////////////////////////////
const PdServ::Signal* Variable::signal() const
{
    return dynamic_cast<const PdServ::Signal*>(variable);
}

/////////////////////////////////////////////////////////////////////////////
const PdServ::Parameter* Variable::parameter() const
{
    return dynamic_cast<const PdServ::Parameter*>(variable);
}

/////////////////////////////////////////////////////////////////////////////
void Variable::setAttributes(XmlElement &element, bool shortReply) const
{
    // name=
    // value=
    // index=
    XmlElement::Attribute(element, "index") << variableIndex;
    XmlElement::Attribute(element, "name").setEscaped(path().c_str());
 
    if (shortReply)
        return;

    // alias=
    // unit=
    // comment=
    if (!variable->alias.empty())
        XmlElement::Attribute(element, "alias") << variable->alias;
    if (!variable->unit.empty())
        XmlElement::Attribute(element, "unit") << variable->unit;
    if (!variable->comment.empty())
        XmlElement::Attribute(element, "comment") << variable->comment;
 
    // datasize=
    XmlElement::Attribute(element, "datasize") << variable->width;
 
    // typ=
    const char *dtype = 0;
    switch (variable->dtype) {
        case PdServ::Variable::boolean_T: dtype = "TCHAR";     break;
        case PdServ::Variable::uint8_T:   dtype = "TUCHAR";    break;
        case PdServ::Variable::int8_T:    dtype = "TCHAR";     break;
        case PdServ::Variable::uint16_T:  dtype = "TUSHORT";   break;
        case PdServ::Variable::int16_T:   dtype = "TSHORT";    break;
        case PdServ::Variable::uint32_T:  dtype = "TUINT";     break;
        case PdServ::Variable::int32_T:   dtype = "TINT";      break;
        case PdServ::Variable::uint64_T:  dtype = "TULINT";    break;
        case PdServ::Variable::int64_T:   dtype = "TLINT";     break;
        case PdServ::Variable::single_T:  dtype = "TFLT";      break;
        case PdServ::Variable::double_T:  dtype = "TDBL";      break;
        default:                                break;
    }
    if (nelem > 1)
        XmlElement::Attribute(element, "typ")
            << dtype
            << (variable->ndims == 1 ? "_LIST" : "_MATRIX");
    else
        XmlElement::Attribute(element, "typ") << dtype;
 
    // For vectors:
    // anz=
    // cnum=
    // rnum=
    // orientation=
    if (nelem > 1) {
        XmlElement::Attribute(element, "anz") << nelem;
        const char *orientation;
        size_t cnum, rnum;

        // Transmit either a vector or a matrix
        if (variable->ndims == 1) {
            cnum = variable->nelem;
            rnum = 1;
            orientation = "VECTOR";
        }
        else {
            const size_t *dim = variable->getDim();
            cnum = dim[variable->ndims - 1];
            rnum = variable->nelem / cnum;
            orientation = "MATRIX_ROW_MAJOR";
        }

        XmlElement::Attribute(element, "cnum") << cnum;
        XmlElement::Attribute(element, "rnum") << rnum;
        XmlElement::Attribute(element, "orientation") << orientation;
    }
 
    // unit=
    if (!variable->unit.empty())
        XmlElement::Attribute(element, "unit") << variable->unit;

    // text=
    if (!variable->comment.empty())
        XmlElement::Attribute(element, "text") << variable->comment;

    // hide=
    // unhide=
}

/////////////////////////////////////////////////////////////////////////////
void Variable::toCSV(std::ostream& os,
        const void *data, size_t nblocks, size_t precision) const
{
    size_t idx = 0;
    for (size_t n = 0; n < nblocks; ++n) {
        for (size_t i = 0; i < nelem; ++i) {
            if (idx)
                os << ',';
            printFunc(os, data, idx++, precision);
        }
    }
}
