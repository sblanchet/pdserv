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

#include "Channel.h"
#include "XmlDoc.h"
#include "../Task.h"
#include "../Signal.h"

#include <sstream>

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Channel::Channel( const HRTLab::Signal *s, unsigned int index,
        unsigned int sigOffset):
    index(index), signal(s), nelem(1), memSize(s->width),
    bufferOffset(sigOffset * s->width)
{
    cout << __PRETTY_FUNCTION__ << index << endl;
    ///cout << s->path << '[' << endl;

    std::ostringstream os;
    const size_t *idx = signal->getDim();
    size_t r = signal->nelem;
    size_t n = sigOffset;
    size_t x;

    while (r > nelem) {
        r /= *idx++;
        x = n / r;
        n -= x*r;

        os << '/' << x;
    }

    extension = os.str();

    cout << s->path << '[' << index << "] = " << path() << endl;
}

/////////////////////////////////////////////////////////////////////////////
Channel::Channel( const HRTLab::Signal *s, unsigned int index):
    index(index), signal(s), nelem(s->nelem), memSize(s->memSize),
    bufferOffset(0)
{
}

/////////////////////////////////////////////////////////////////////////////
Channel::~Channel()
{
}

/////////////////////////////////////////////////////////////////////////////
void Channel::setXmlAttributes( MsrXml::Element *element,
        bool shortReply, const char *data) const
{
    // <channel name="/lan/World Time" alias="" index="0" typ="TDBL"
    //   datasize="8" bufsize="500" HZ="50" unit="" value="1283134199.93743"/>
    double freq = 1.0 / signal->task->sampleTime / signal->decimation;

    // The MSR protocoll wants a bufsize, the maximum number of
    // values that can be retraced. This artificial limitation does
    // not exist any more. Instead, choose a buffer size so that
    // at a maximum of 1 second has to be stored.
    size_t bufsize = std::max( 1U, (size_t)(freq + 0.5));


    element->setVariableAttributes(signal, index, path(), nelem, shortReply);

    if (shortReply)
        return;

    // bufsize=
    element->setAttribute("bufsize", bufsize);
    element->setAttribute("HZ", freq);

    element->csvValueAttr("value", data + bufferOffset, signal, nelem);
}

/////////////////////////////////////////////////////////////////////////////
std::string Channel::path() const
{
    return signal->path + extension;
}
