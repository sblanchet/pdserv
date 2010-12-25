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
    cout << __PRETTY_FUNCTION__ << index << endl;
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

    cout << p->path << '[' << index << "] = " << path() << endl;
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
bool Parameter::setValue(const char *buf) const
{
    char value[mainParam->memSize];

    mainParam->getValue(value);
    std::copy(buf, buf + memSize, value + bufferOffset);
    return mainParam->setValue(value);
}

