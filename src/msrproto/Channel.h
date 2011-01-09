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

#ifndef MSRCHANNEL_H
#define MSRCHANNEL_H

#include <string>
#include "PrintVariable.h"

namespace HRTLab {
    class Signal;
    class Receiver;
}

namespace MsrXml {
    class Element;
}

namespace MsrProto {

class DirectoryNode;

class Channel {
    public:
        Channel( const DirectoryNode *directory,
                const HRTLab::Signal *s,
                unsigned int channelIndex,
                unsigned int signalOffset,
                unsigned int nelem);
        ~Channel();

        void setXmlAttributes(MsrXml::Element*, bool shortReply,
                    const char *signalBuf) const;

        std::string path() const;

        const DirectoryNode * const directory;
        const unsigned int index;
        const HRTLab::Signal * const signal;
        const size_t nelem;
        const size_t memSize;
        const size_t bufferOffset;

        const PrintFunc printFunc;

    private:
};

}

#endif //MSRCHANNEL_H
