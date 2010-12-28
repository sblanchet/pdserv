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
        unsigned int index, unsigned int sigOffset, size_t nelem):
    index(index),
    mainParam(p), nelem(nelem), memSize(p->width),
    bufferOffset(sigOffset * p->width),
    printFunc(getPrintFunc(p->dtype)),
    persistent(false)
{
    //cout << __PRETTY_FUNCTION__ << index << endl;
    ///cout << s->path << '[' << endl;

    std::ostringstream os;
    const size_t *idx = mainParam->getDim();
    size_t r = mainParam->nelem;
    size_t n = sigOffset;
    size_t x;

    while (r > nelem) {
        r /= *idx++;
        x = n / r;
        n -= x*r;

        os << '/' << x;
    }

    extension = os.str();

    //cout << p->path << '[' << index << "] = " << path() << endl;
}

/////////////////////////////////////////////////////////////////////////////
Parameter::Parameter( const HRTLab::Parameter *p, unsigned int index):
    index(index), mainParam(p), nelem(p->nelem), memSize(p->memSize),
    bufferOffset(0), printFunc(getPrintFunc(p->dtype)),
    persistent(false)
{
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
std::string Parameter::path() const
{
    return mainParam->path + extension;
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
    if (startindex >= nelem)
        return -ERANGE;

    char value[mainParam->memSize];
    static const char hexNum[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,
        0,10,11,12,13,14,15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0,10,11,12,13,14,15
    };

    // Copy the original contents
    mainParam->getValue(value);

    char *valueStart = value + bufferOffset + mainParam->width * startindex;
    char *valueEnd = valueStart + memSize;
    for (char *c = valueStart; *s and c < valueEnd; c++) {
        unsigned char c1 = *s++ - '0';
        unsigned char c2 = *s++ - '0';
        if (std::max(c1,c2) >= sizeof(hexNum))
            return -EINVAL;
        *c = hexNum[c1] << 4 | hexNum[c2];
    }
    // FIXME: actually the setting operation must also check for
    // endianness!

    return mainParam->setValue(value);
}

/////////////////////////////////////////////////////////////////////////////
class Convert {
    public:
        Convert(si_datatype_t t, char *buf): buf(buf) {
            switch (t) {
                case si_boolean_T: conv = setTo<bool>;     break;
                case si_uint8_T:   conv = setTo<uint8_t>;  break;
                case si_sint8_T:   conv = setTo<int8_t>;   break;
                case si_uint16_T:  conv = setTo<uint16_t>; break;
                case si_sint16_T:  conv = setTo<int16_t>;  break;
                case si_uint32_T:  conv = setTo<uint32_t>; break;
                case si_sint32_T:  conv = setTo<int32_t>;  break;
                case si_uint64_T:  conv = setTo<uint64_t>; break;
                case si_sint64_T:  conv = setTo<int64_t>;  break;
                case si_single_T:  conv = setTo<float>;    break;
                case si_double_T:  conv = setTo<double>;   break;
                default:           conv = 0;               break;
            }
        }
        void set(size_t idx, double val) {
            (*conv)(buf, idx, val);
        }

    private:
        char * const buf;
        void (*conv)(char *, size_t, double);

        template<class T>
        static void setTo(char *dst, size_t idx, double src) {
            reinterpret_cast<T*>(dst)[idx] = src;
        }
};

/////////////////////////////////////////////////////////////////////////////
int Parameter::setDoubleValue(const char *buf, size_t startindex) const
{
    if (startindex >= nelem)
        return -ERANGE;

    char value[mainParam->memSize];
    mainParam->getValue(value);

    std::istringstream is(buf);
    is.imbue(std::locale::classic());
    Convert converter(mainParam->dtype,
            value + bufferOffset + startindex * mainParam->width);

    char c;
    double v;
    for (size_t i = 0; i < nelem - startindex; i++) {
        if (i) {
            is >> c;
            if (c != ',' and c != ';' and !isspace(c))
                return -EINVAL;
        }

        is >> v;

        if (!is)
            break;

        converter.set(i, v);
    }

    return mainParam->setValue(value);
}

