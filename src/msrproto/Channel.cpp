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

#include "Channel.h"
#include "Directory.h"
#include "XmlDoc.h"
#include "../Signal.h"

#include <sstream>

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Channel::Channel( const DirectoryNode *directory, const PdServ::Signal *s,
        unsigned int channelIndex, unsigned int index, unsigned int nelem):
    Variable(directory, s, channelIndex, index, nelem),
    signal(s)
{

    //cout << __PRETTY_FUNCTION__ << index << endl;
//    cout << path() << ' ' << index << ' ' << sigOffset << endl;

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
    double freq = 1.0 / signal->sampleTime;

    // The MSR protocoll wants a bufsize, the maximum number of
    // values that can be retraced. This artificial limitation does
    // not exist any more. Instead, choose a buffer size so that
    // at a maximum of 10 seconds has to be stored.
    size_t bufsize = 10 * std::max( 1U, (unsigned int)(freq + 0.5));


    setVariableAttributes(
            element, signal, index, directory->path(), nelem, shortReply);

    if (shortReply)
        return;

    // bufsize=
    element->setAttribute("bufsize", bufsize);
    element->setAttribute("HZ", freq);

    csvAttribute(element, "value",
            printFunc, signal, nelem, data + bufferOffset);
}
