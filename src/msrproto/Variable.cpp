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
#include "XmlElement.h"
#include "Directory.h"
#include "../Signal.h"
#include "../Parameter.h"
#include "../DataType.h"

#include <cstdio>
#include <stdint.h>
#include <sstream>

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Variable::Variable(const Server *server, const PdServ::Variable *v,
        unsigned int variableIndex, unsigned int elementIndex):
    DirectoryNode(server),
    elementIndex(elementIndex != ~0U ? elementIndex : 0),
    variable(v),
    variableIndex(variableIndex),
    nelem(elementIndex == ~0U ? v->dim.nelem : 1),
    memSize(nelem * v->dtype.size)
{
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
    XmlElement::Attribute(element, "datasize") << variable->dtype.size;

    // typ=
    const char *dtype;
    switch (variable->dtype.primary()) {
        case PdServ::DataType::boolean_T : dtype = "TCHAR";      break;
        case PdServ::DataType::  uint8_T : dtype = "TUCHAR";    break;
        case PdServ::DataType::   int8_T : dtype = "TCHAR";     break;
        case PdServ::DataType:: uint16_T : dtype = "TUSHORT";   break;
        case PdServ::DataType::  int16_T : dtype = "TSHORT";    break;
        case PdServ::DataType:: uint32_T : dtype = "TUINT";     break;
        case PdServ::DataType::  int32_T : dtype = "TINT";      break;
        case PdServ::DataType:: uint64_T : dtype = "TULINT";    break;
        case PdServ::DataType::  int64_T : dtype = "TLINT";     break;
        case PdServ::DataType:: double_T : dtype = "TDBL";      break;
        case PdServ::DataType:: single_T : dtype = "TFLT";      break;
        default                          : dtype = "COMPOUND";  break;
    }
    if (nelem > 1)
        XmlElement::Attribute(element, "typ")
            << dtype
            << (variable->dim.size() == 1 ? "_LIST" : "_MATRIX");
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
        if (variable->dim.size() == 1) {
            cnum = variable->dim.nelem;
            rnum = 1;
            orientation = "VECTOR";
        }
        else {
            cnum = variable->dim.back();
            rnum = variable->dim.nelem / cnum;
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
