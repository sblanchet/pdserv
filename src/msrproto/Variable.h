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

#ifndef MSRVARIABLE_H
#define MSRVARIABLE_H

#include <string>
#include <ostream>
#include "../Variable.h"
#include "Directory.h"

namespace PdServ {
    class Variable;
    class Signal;
    class Parameter;
}

namespace MsrProto {

class DirectoryNode;
class XmlElement;

class Variable: public DirectoryNode {
    public:
        Variable( const PdServ::Variable *v,
                unsigned int variableIndex,
                unsigned int elementIndex);
        virtual ~Variable();

        const PdServ::Signal* signal() const;
        const PdServ::Parameter* parameter() const;

        const unsigned int elementIndex;
        const PdServ::Variable * const variable;

        const unsigned int variableIndex;
        const size_t nelem;
        const size_t memSize;
//        const size_t bufferOffset;

        void setAttributes(XmlElement &element, bool shortReply) const;

        void toCSV(std::ostream& os,
                const void *buf, size_t nblocks, size_t precision) const;

    private:
};

}

#endif //MSRVARIABLE_H
