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

#include "Parameter.h"
#include "XmlElement.h"
#include "Directory.h"
#include "Session.h"
#include "../Parameter.h"
#include "../Debug.h"

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
Parameter::Parameter( const DirectoryNode *directory,
        unsigned int variableIndex, const PdServ::Parameter *p,
        unsigned int elementIndex):
    Variable(directory, p, variableIndex, elementIndex),
    mainParam(p),
    dependent(dependent),
    persistent(false)
{
    //cout << __PRETTY_FUNCTION__ << index << endl;
    switch (mainParam->dtype) {
        case PdServ::Variable::boolean_T: append = setTo<bool>;     break;
        case PdServ::Variable::uint8_T:   append = setTo<uint8_t>;  break;
        case PdServ::Variable::int8_T:    append = setTo<int8_t>;   break;
        case PdServ::Variable::uint16_T:  append = setTo<uint16_t>; break;
        case PdServ::Variable::int16_T:   append = setTo<int16_t>;  break;
        case PdServ::Variable::uint32_T:  append = setTo<uint32_t>; break;
        case PdServ::Variable::int32_T:   append = setTo<int32_t>;  break;
        case PdServ::Variable::uint64_T:  append = setTo<uint64_t>; break;
        case PdServ::Variable::int64_T:   append = setTo<int64_t>;  break;
        case PdServ::Variable::single_T:  append = setTo<float>;    break;
        case PdServ::Variable::double_T:  append = setTo<double>;   break;
        default:           append = 0;               break;
    }
}

/////////////////////////////////////////////////////////////////////////////
Parameter::~Parameter()
{
}

/////////////////////////////////////////////////////////////////////////////
void Parameter::addChild(const Parameter *p)
{
    if (children.empty())
        children.reserve(nelem);
    children.push_back(p);
}

/////////////////////////////////////////////////////////////////////////////
void Parameter::setXmlAttributes( Session *s, XmlElement &element,
        bool shortReply, bool hex, bool writeAccess, size_t precision) const
{
    struct timespec mtime;
    char valueBuf[memSize];
    unsigned int flags = writeAccess
        ? MSR_R | MSR_W | MSR_WOP : MSR_R;

    mainParam->getValue(s, valueBuf, elementIndex, nelem, &mtime);

    // <parameter name="/lan/Control/EPC/EnableMotor/Value/2"
    //            index="30" value="0"/>

    setAttributes(element, shortReply);

    if (!shortReply) {
        element.setAttribute("flags", flags + (dependent ? 0x100 : 0));

        // persistent=
        if (persistent)
            element.setAttribute("persistent", persistent);
    }

    // mtime=
    element.setAttribute("mtime", mtime);

    if (hex)
        element.hexDecAttribute("hexvalue", valueBuf, memSize);
    else
        element.csvAttribute("value", this, valueBuf, 1, precision);

    return;
}

/////////////////////////////////////////////////////////////////////////////
void Parameter::getValue(const Session *session, char *buf) const
{
    mainParam->getValue(session, buf, elementIndex, nelem);
}

/////////////////////////////////////////////////////////////////////////////
int Parameter::setHexValue(const Session *session,
        const char *s, size_t startindex, size_t &count) const
{
    char valueBuf[memSize];
    const char *valueEnd = valueBuf + memSize;
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

    count = (c - valueBuf) / mainParam->width;
    return mainParam->setValue(
            session, valueBuf, elementIndex + startindex, count);
}

/////////////////////////////////////////////////////////////////////////////
int Parameter::setDoubleValue(const Session *session,
        const char *buf, size_t startindex, size_t &count) const
{
    char value[memSize];
    char *dataBuf = value;
    std::istringstream is(buf);
    is.imbue(std::locale::classic());

    char c;
    double v;
    for (count = 0; count < nelem - startindex; count++) {
        if (count) {
            is >> c;
            if (c != ',' and c != ';' and !isspace(c))
                return -EINVAL;
        }

        is >> v;

        if (!is)
            break;

        (*append)(dataBuf, v);
    }

    return mainParam->setValue(
            session, value, elementIndex + startindex, count);
}

/////////////////////////////////////////////////////////////////////////////
void Parameter::valueChanged(
        std::ostream& os, size_t start, size_t nelem) const
{
//    XmlElement pu("pu");
//
//    pu.setAttribute("index", variableIndex);
//    os << pu;
//
//    while (variableIndex + start < children.size() and nelem--) {
//        pu.setAttribute("index", variableIndex + start++);
//        os << pu;
//    }
//
//    os << std::flush;
}
