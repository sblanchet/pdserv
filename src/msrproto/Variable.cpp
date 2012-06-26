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
void Variable::setDataType(XmlElement &element, const PdServ::DataType& dtype,
        const PdServ::DataType::DimType& dim) const
{
    // datasize=
    XmlElement::Attribute(element, "datasize") << dtype.size;

    // typ=
    const char *dtstr;
    switch (dtype.primary()) {
        case PdServ::DataType::boolean_T : dtstr = "TCHAR";      break;
        case PdServ::DataType::  uint8_T : dtstr = "TUCHAR";    break;
        case PdServ::DataType::   int8_T : dtstr = "TCHAR";     break;
        case PdServ::DataType:: uint16_T : dtstr = "TUSHORT";   break;
        case PdServ::DataType::  int16_T : dtstr = "TSHORT";    break;
        case PdServ::DataType:: uint32_T : dtstr = "TUINT";     break;
        case PdServ::DataType::  int32_T : dtstr = "TINT";      break;
        case PdServ::DataType:: uint64_T : dtstr = "TULINT";    break;
        case PdServ::DataType::  int64_T : dtstr = "TLINT";     break;
        case PdServ::DataType:: double_T : dtstr = "TDBL";      break;
        case PdServ::DataType:: single_T : dtstr = "TFLT";      break;
        default                          : dtstr = "COMPOUND";  break;
    }
    if (dim.nelem > 1)
        XmlElement::Attribute(element, "typ")
            << dtstr
            << (dim.size() == 1 ? "_LIST" : "_MATRIX");
    else
        XmlElement::Attribute(element, "typ") << dtstr;

    // For vectors:
    // anz=
    // cnum=
    // rnum=
    // orientation=
    if (dim.nelem > 1) {
        XmlElement::Attribute(element, "anz") << dim.nelem;
        const char *orientation;
        size_t cnum, rnum;

        // Transmit either a vector or a matrix
        if (dim.size() == 1) {
            cnum = dim.nelem;
            rnum = 1;
            orientation = "VECTOR";
        }
        else {
            cnum = dim.back();
            rnum = dim.nelem / cnum;
            orientation = "MATRIX_ROW_MAJOR";
        }

        XmlElement::Attribute(element, "cnum") << cnum;
        XmlElement::Attribute(element, "rnum") << rnum;
        XmlElement::Attribute(element, "orientation") << orientation;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Variable::addCompoundFields(XmlElement &element,
        const PdServ::DataType& dtype) const
{
    const PdServ::DataType::FieldList& fieldList = dtype.getFieldList();

    for (PdServ::DataType::FieldList::const_iterator it = fieldList.begin();
            it != fieldList.end(); ++it) {
        XmlElement field("field", element);
        XmlElement::Attribute(field, "name").setEscaped((*it)->name.c_str());
        XmlElement::Attribute(field, "offset") << (*it)->offset;

        setDataType(element, (*it)->type, (*it)->dim);

        addCompoundFields(field, (*it)->type);
    }
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

    setDataType(element, variable->dtype, variable->dim);

    // unit=
    if (!variable->unit.empty())
        XmlElement::Attribute(element, "unit") << variable->unit;

    // text=
    if (!variable->comment.empty())
        XmlElement::Attribute(element, "text") << variable->comment;

    // hide=
    // unhide=
}
