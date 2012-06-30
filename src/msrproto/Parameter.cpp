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

#include "Parameter.h"
#include "XmlElement.h"
#include "Directory.h"
#include "Session.h"
#include "OStream.h"
#include "../Parameter.h"
#include "../Debug.h"
#include "../DataType.h"

#include <sstream>
#include <cerrno>
#include <stdint.h>

#define MSR_R   0x01    /* Parameter is readable */
#define MSR_W   0x02    /* Parameter is writeable */
#define MSR_WOP 0x04    /* Parameter is writeable in real-time */
//#define MSR_MONOTONIC 0x8 /* List must be monotonic */
//#define MSR_S   0x10    /* Parameter must be saved by clients */
#define MSR_G   0x20    /* Gruppenbezeichnung (unused) */
#define MSR_AW  0x40    /* Writeable by admin only */
#define MSR_P   0x80    /* Persistant parameter, written to disk */
#define MSR_DEP 0x100   /* This parameter is an exerpt of another parameter.
                           Writing to this parameter also causes an update
                           notice for the encompassing parameter */
#define MSR_AIC 0x200   /* Asynchronous input channel */

/* ein paar Kombinationen */
#define MSR_RWS (MSR_R | MSR_W | MSR_S)
#define MSR_RP (MSR_R | MSR_P)


using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Parameter::Parameter( const PdServ::Parameter *p,
        const std::string& name, DirectoryNode* dir,
        unsigned int variableIndex, bool traditional):
    Variable(p, name, dir, variableIndex),
    mainParam(p),
    dependent(false),
    persistent(false)
{
    createChildren(traditional);
}

/////////////////////////////////////////////////////////////////////////////
Parameter::Parameter(const Parameter *parent, const std::string& name,
        DirectoryNode *dir, const PdServ::DataType& dtype,
        size_t nelem, size_t offset):
    Variable(parent, name, dir, dtype, nelem, offset),
    mainParam(parent->mainParam),
    dependent(true),
    persistent(parent->persistent)
{
    //log_debug("new var %s %zu @%zu", name.c_str(), nelem, offset);
    createChildren(true);
}

/////////////////////////////////////////////////////////////////////////////
const Variable* Parameter::createChild(DirectoryNode* dir,
                const std::string& name,
                const PdServ::DataType& dtype, size_t nelem, size_t offset)
{
    return new Parameter(this, name, dir, dtype, nelem, offset);
}

/////////////////////////////////////////////////////////////////////////////
void Parameter::setXmlAttributes(XmlElement &element, const char *valueBuf,
        struct timespec const& mtime, bool shortReply, bool hex,
        size_t precision, const std::string& id) const
{
    unsigned int flags = MSR_R | MSR_W | MSR_WOP;

    // <parameter name="/lan/Control/EPC/EnableMotor/Value/2"
    //            index="30" value="0"/>

    setAttributes(element, shortReply);

    if (!shortReply) {
        XmlElement::Attribute(element, "flags")
            << flags + (dependent ? 0x100 : 0);

        // persistent=
        if (persistent)
            XmlElement::Attribute(element, "persistent") << persistent;
    }

    // mtime=
    XmlElement::Attribute(element, "mtime") << mtime;

    if (hex)
        XmlElement::Attribute(element, "hexvalue")
            .hexDec(valueBuf + offset, dtype.size);
    else
        XmlElement::Attribute(element, "value").csv( this,
                valueBuf + offset, 1, precision);

    if (!id.empty())
        XmlElement::Attribute(element, "id").setEscaped(id.c_str());

    addCompoundFields(element, variable->dtype);

    return;
}

/////////////////////////////////////////////////////////////////////////////
int Parameter::setHexValue(const Session *session,
        const char *s, size_t startindex, size_t &count) const
{
    const size_t len = dtype.size * count;
    char valueBuf[len];
    const char *valueEnd = valueBuf + len;
    static const char hexNum[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,
        0,10,11,12,13,14,15, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0,10,11,12,13,14,15
    };

    char *c;
    for (c = valueBuf; *s and c < valueEnd; c++) {
        unsigned char c1 = *s++ - '0';
        unsigned char c2 = *s++ - '0';
        if (std::max(c1,c2) >= sizeof(hexNum))
            return -EINVAL;
        *c = hexNum[c1] << 4 | hexNum[c2];
    }
    // FIXME: actually the setting operation must also check for
    // endianness!

    count = (c - valueBuf) / mainParam->dtype.size;
    return mainParam->setValue( session, valueBuf,
            offset + startindex * dtype.size, len);
}

/////////////////////////////////////////////////////////////////////////////
int Parameter::setElements(std::istream& is,
        const PdServ::DataType& dtype, const PdServ::DataType::DimType& dim,
        char*& buf, size_t& count) const
{
    if (dtype.primary() == dtype.compound_T) {
        const PdServ::DataType::FieldList& fieldList = dtype.getFieldList();
        PdServ::DataType::FieldList::const_iterator it;
        for (size_t i = 0; i < dim.nelem; ++i) {
            for (it = fieldList.begin(); it != fieldList.end(); ++it) {
                int rv = setElements(is, (*it)->type, (*it)->dim, buf, count);
                if (rv)
                    return rv;
            }
        }
        return 0;
    }

    double v;
    char c;
    for (size_t i = 0; i < dim.nelem; ++i) {

        is >> v;
        if (!is)
            return -EINVAL;

        dtype.setValue(buf, v);
        count++;

        is >> c;
        if (!is)
            return 1;

        if (c != ',' and c != ';' and !isspace(c))
            return -EINVAL;
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
int Parameter::setDoubleValue(const Session *session,
        const char *buf, size_t startindex, size_t &count) const
{
    char valueBuf[memSize];
    char *dataBuf = valueBuf;

    std::istringstream is(buf);
    is.imbue(std::locale::classic());

    //log_debug("buf=%s", buf);
    count = 0;
    int rv = setElements(is, dtype, dim, dataBuf, count);

//    log_debug("rv=%i startindex=%zu offset=%zu count=%zu",
//            rv, startindex, offset/dtype.size + startindex, count);
//    for (size_t i = 0; i < count; i++)
//        std::cerr << ((const double*)valueBuf)[i] << ' ';
//    std::cerr << std::endl;
    return rv < 0
        ? rv
        : mainParam->setValue( session, valueBuf,
                offset/dtype.size + startindex, count);
}

/////////////////////////////////////////////////////////////////////////////
void Parameter::valueChanged( ostream &xmlstream,
        size_t start, size_t nelem) const
{
     {
         ostream::locked ls(xmlstream);
         XmlElement pu("pu", ls);
         XmlElement::Attribute(pu, "index") << variableIndex;
     }
 //    log_debug("start=%zu nelem=%zu", start, nelem);
 
     const Variable::List *children = getChildren();
     if (!children)
         return;
 
     Variable::List::const_iterator it = children->begin();
     while (start--)
         it++;
     while (nelem--) {
         ostream::locked ls(xmlstream);
         XmlElement pu("pu", ls);
         XmlElement::Attribute(pu, "index") << (*it++)->variableIndex;
     }
}
