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
#include <list>
#include <ostream>
#include "../Variable.h"
#include "DirectoryNode.h"

namespace PdServ {
    class Variable;
    class Signal;
    class Parameter;
}

namespace MsrProto {

class XmlElement;

class Variable: public DirectoryNode {
    public:
        Variable(const PdServ::Variable *v, size_t index,
                const PdServ::DataType& dtype,
                const PdServ::DataType::DimType& dim, size_t offset,
                Variable* parent);
        ~Variable();

        const PdServ::Variable * const variable;

        const size_t index;
        const PdServ::DataType& dtype;
        const PdServ::DataType::DimType dim;
        const size_t offset;
        const size_t memSize;
        bool hidden;

        void setAttributes(XmlElement &element, bool shortReply) const;
        void addCompoundFields(XmlElement &element,
                const PdServ::DataType& ) const;

        typedef std::list<const Variable*> List;
        const List* getChildren() const;

    private:
        void setDataType(XmlElement &element, const PdServ::DataType& dtype,
                const PdServ::DataType::DimType& dim) const;

        List* children;

        void addChild(const Variable* child);
};

}

#endif //MSRVARIABLE_H
