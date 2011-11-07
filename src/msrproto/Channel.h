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

#ifndef MSRCHANNEL_H
#define MSRCHANNEL_H

#include <string>
#include "Variable.h"

namespace PdServ {
    class Signal;
}

namespace MsrProto {

class DirectoryNode;

class Channel: public Variable {
    public:
        Channel( const DirectoryNode *directory,
                const PdServ::Signal *s,
                unsigned int channelIndex,
                unsigned int index,
                unsigned int nelem);
        ~Channel();

        void setXmlAttributes(MsrXml::Element*, bool shortReply,
                    const char *signalBuf) const;

        const PdServ::Signal *signal;
    private:
};

}

#endif //MSRCHANNEL_H
