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

#ifndef MSRCHANNEL_H
#define MSRCHANNEL_H

#include <string>
#include "Variable.h"

namespace PdServ {
    class Signal;
}

namespace MsrProto {

class XmlElement;

class Channel: public Variable {
    public:
        Channel(const PdServ::Signal *s,
                const std::string& name, DirectoryNode* parent,
                unsigned int channelIndex, bool traditional);

        Channel(const Channel *c,
                const std::string& name, DirectoryNode* parent,
                const PdServ::DataType& dtype, size_t nelem, size_t offset);

        void setXmlAttributes(XmlElement&, bool shortReply,
                    const char *signalBuf, size_t precision) const;

        const PdServ::Signal *signal;

    private:
        // Reimplemented from Variable
        const Variable* createChild(
                DirectoryNode* dir, const std::string& path,
                const PdServ::DataType& dtype, size_t nelem, size_t offset);
};

}

#endif //MSRCHANNEL_H
