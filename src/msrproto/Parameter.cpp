/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdcomserv package.
 *
 *  pdcomserv is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pdcomserv is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pdcomserv. See COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include "config.h"

#include "Parameter.h"
#include "XmlDoc.h"
#include "PrintVariable.h"
#include "Directory.h"
#include "../Task.h"
#include "../Parameter.h"

#include <sstream>
#include <cerrno>
#include <stdint.h>

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Parameter::Parameter( const HRTLab::Parameter *p,
        unsigned int index, unsigned int nelem, size_t parameterElement):
    index(index), mainParam(p), nelem(nelem), memSize(p->width),
    bufferOffset(parameterElement * p->width),
    printFunc(getPrintFunc(p->dtype)),
    converter(this), persistent(false)
{
    //cout << __PRETTY_FUNCTION__ << index << endl;
}

/////////////////////////////////////////////////////////////////////////////
Parameter::~Parameter()
{
}

/////////////////////////////////////////////////////////////////////////////
void Parameter::setXmlAttributes( MsrXml::Element *element,
        bool shortReply, bool hex, unsigned int flags) const
{
    struct timespec mtime;
    char data[mainParam->memSize];

    mainParam->getValue(data, &mtime);

    // <parameter name="/lan/Control/EPC/EnableMotor/Value/2"
    //            index="30" value="0"/>

    setVariableAttributes(element, mainParam, index, path(), nelem, shortReply);

    if (!shortReply) {
        element->setAttribute("flags",
                flags + (mainParam->nelem != nelem ? 0x100 : 0));

        // persistent=
        element->setAttribute("persistent", persistent);
    }

    // mtime=
    element->setAttribute("mtime", mtime);

    if (hex)
        hexDecAttribute(element, "hexvalue",
                mainParam, nelem, data + bufferOffset);
    else
        csvAttribute(element, "value",
                printFunc, mainParam, nelem, data + bufferOffset);
    return;

}

/////////////////////////////////////////////////////////////////////////////
void Parameter::setNode(const DirectoryNode *n, const char *name)
{
    node = n;
}

/////////////////////////////////////////////////////////////////////////////
std::string Parameter::path() const
{
    return node->path();
}

/////////////////////////////////////////////////////////////////////////////
void Parameter::getValue(char *buf) const
{
    char value[mainParam->memSize];

    mainParam->getValue(value);
    std::copy(value + bufferOffset, value + bufferOffset + memSize, buf);
}

/////////////////////////////////////////////////////////////////////////////
int Parameter::setHexValue(const char *s, size_t startindex) const
{
    int rv;
    char value[mainParam->memSize];
    converter.setbuf(value);
    if ((rv = converter.readHexValue(s, startindex)))
        return rv;

    return mainParam->setValue(value);
}

/////////////////////////////////////////////////////////////////////////////
int Parameter::setDoubleValue(const char *buf, size_t startindex) const
{
    int rv;
    char value[mainParam->memSize];
    converter.setbuf(value);
    if ((rv = converter.readDoubleList(buf, startindex)))
        return rv;

    return mainParam->setValue(value);
}

/////////////////////////////////////////////////////////////////////////////
Parameter::Converter::Converter(Parameter *p): parameter(p) {
    switch (parameter->mainParam->dtype) {
        case si_boolean_T: append = setTo<bool>;     break;
        case si_uint8_T:   append = setTo<uint8_t>;  break;
        case si_sint8_T:   append = setTo<int8_t>;   break;
        case si_uint16_T:  append = setTo<uint16_t>; break;
        case si_sint16_T:  append = setTo<int16_t>;  break;
        case si_uint32_T:  append = setTo<uint32_t>; break;
        case si_sint32_T:  append = setTo<int32_t>;  break;
        case si_uint64_T:  append = setTo<uint64_t>; break;
        case si_sint64_T:  append = setTo<int64_t>;  break;
        case si_single_T:  append = setTo<float>;    break;
        case si_double_T:  append = setTo<double>;   break;
        default:           append = 0;               break;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Parameter::Converter::setbuf(char *b) const
{
    parameter->mainParam->getValue(b);
    dataBuf = b + parameter->bufferOffset;
}

/////////////////////////////////////////////////////////////////////////////
int Parameter::Converter::readDoubleList(const char *buf, size_t startindex) const
{
    if (startindex >= parameter->nelem)
        return -ERANGE;

    dataBuf += startindex * parameter->mainParam->width;

    std::istringstream is(buf);
    is.imbue(std::locale::classic());

    char c;
    double v;
    for (size_t i = 0; i < parameter->nelem - startindex; i++) {
        if (i) {
            is >> c;
            if (c != ',' and c != ';' and !isspace(c))
                return -EINVAL;
        }

        is >> v;

        if (!is)
            break;

        (*append)(dataBuf, v);
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
int Parameter::Converter::readHexValue(const char *s, size_t startindex) const
{
    if (startindex >= parameter->nelem)
        return -ERANGE;

    static const char hexNum[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,
        0,10,11,12,13,14,15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0,10,11,12,13,14,15
    };

    char *valueStart = dataBuf + startindex * parameter->mainParam->width;
    char *valueEnd = valueStart + parameter->memSize;
    for (char *c = valueStart; *s and c < valueEnd; c++) {
        unsigned char c1 = *s++ - '0';
        unsigned char c2 = *s++ - '0';
        if (std::max(c1,c2) >= sizeof(hexNum))
            return -EINVAL;
        *c = hexNum[c1] << 4 | hexNum[c2];
    }
    // FIXME: actually the setting operation must also check for
    // endianness!

    return 0;
}
