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

#ifndef MSRVARIABLE_H
#define MSRVARIABLE_H

#include <string>
#include "PrintVariable.h"

namespace PdServ {
    class Variable;
    class Signal;
    class Parameter;
}

namespace MsrXml {
    class Element;
}

namespace MsrProto {

class DirectoryNode;

class Variable {
    public:
        Variable( const DirectoryNode *directory,
                const PdServ::Variable *v,
                unsigned int index,
                unsigned int element,
                unsigned int nelem);
        ~Variable();

        std::string path() const;
        const PdServ::Signal* signal() const;
        const PdServ::Parameter* parameter() const;

        const DirectoryNode * const directory;
        const unsigned int index;
        const PdServ::Variable * const variable;
        const size_t nelem;
        const size_t memSize;
        const size_t bufferOffset;
        const size_t variableElement;

        const PrintFunc printFunc;

    private:
};

}

#endif //MSRVARIABLE_H
