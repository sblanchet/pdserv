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
Variable::Variable(const PdServ::Variable *v,
        const std::string& name,
        DirectoryNode* parent,
        unsigned int variableIndex):
    DirectoryNode(name, parent),
    variable(v),
    index(variableIndex),
    dtype(v->dtype),
    dim(v->dim),
    offset(0U),
    memSize(dtype.size * dim.nelem)
{
    children = 0;
}

/////////////////////////////////////////////////////////////////////////////
Variable::Variable(const Variable *v,
        const std::string& name,
        DirectoryNode* dir,
        const PdServ::DataType& dtype,
        size_t nelem, size_t offset):
    DirectoryNode(name, dir),
    variable(v->variable),
    index(v->index + v->childCount() + 1),
    dtype(dtype),
    dim(1, &nelem),
    offset(offset),
    memSize(dtype.size * dim.nelem)
{
    children = 0;
}

/////////////////////////////////////////////////////////////////////////////
Variable::~Variable()
{
    if (children) {
        for (List::const_iterator it = children->begin();
                it != children->end(); ++it)
            delete *it;

        delete children;
    }
}

/////////////////////////////////////////////////////////////////////////////
size_t Variable::childCount() const
{
    if (!children)
        return 0;

    size_t n = children->size();

    for (List::const_iterator it = children->begin();
            it != children->end(); ++it)
        n += (*it)->childCount();

    return n;
}

/////////////////////////////////////////////////////////////////////////////
const Variable::List* Variable::getChildren() const
{
    return children;
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
    XmlElement::Attribute(element, "index") << index;
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

    setDataType(element, dtype, dim);

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
void Variable::createChildren(bool traditional)
{
    if (!traditional
            or (dim.nelem == 1 and dtype.primary() != dtype.compound_T))
        return;

    children = new List;

    if (dtype.primary() == dtype.compound_T or dim.size() > 1)
        createChildren("", dtype, dim, 0, offset);
    else {
        size_t offset = this->offset;
        for (size_t i = 0; i < dim[0]; ++i) {
            std::ostringstream os;

            os << i;

            children->push_back(
                    createChild(this, os.str(), dtype, 1, offset));

            offset += dtype.size;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
size_t Variable::createChildren(const std::string& path,
        const PdServ::DataType& dtype,
        const PdServ::DataType::DimType& dim, size_t dimIdx, size_t offset)
{
    if (dtype.primary() == dtype.compound_T)
        return createCompoundChildren(path, dtype, dim, dimIdx, offset);
    else
        return createVectorChildren(path, dtype, dim, dimIdx, offset);
}

/////////////////////////////////////////////////////////////////////////////
size_t Variable::createVectorChildren(
        const std::string& path, const PdServ::DataType& dtype,
        const PdServ::DataType::DimType& dim, size_t dimIdx, size_t offset)
{
//    log_debug("%s %zu", path.c_str(), dimIdx);
    if (dimIdx != dim.size() - 1) {
        for (size_t i = 0; i < dim[dimIdx]; ++i) {
            std::ostringstream os;
            os << path << '/' << i;
            offset += createChildren(os.str(), dtype, dim, dimIdx + 1, offset);
        }

        return dtype.size * dim.nelem;
    }

    if (dim.back() == 1) {
        children->push_back(
                createChild(this, path.substr(1), dtype, 1, offset));
    }
    else {
        std::string name;
        DirectoryNode* dir = mkdir(path, name);
        children->push_back(
                createChild(dir, name, dtype, dim.back(), offset));
    }

    return dtype.size * dim.back();
}

/////////////////////////////////////////////////////////////////////////////
size_t Variable::createCompoundChildren(
        const std::string& path, const PdServ::DataType& dtype,
        const PdServ::DataType::DimType& dim, size_t dimIdx, size_t offset)
{
//    log_debug("%s %zu", path.c_str(), dim.nelem);
    if (dimIdx != dim.size() and dim.nelem != 1) {
        for (size_t i = 0; i < dim[dimIdx]; ++i) {
            std::ostringstream os;
            os << path << '/' << i;
            offset += createChildren(os.str(), dtype, dim, dimIdx + 1, offset);
        }

        return dtype.size * dim.nelem;
    }

    const PdServ::DataType::FieldList& fieldList = dtype.getFieldList();
    for (PdServ::DataType::FieldList::const_iterator it = fieldList.begin();
            it != fieldList.end(); ++it) {
        createChildren(path + '/' + (*it)->name,
                (*it)->type, (*it)->dim, 0, offset + (*it)->offset);
    }
    return dtype.size;
}
