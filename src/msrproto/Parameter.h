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

#ifndef MSRPARAMETER_H
#define MSRPARAMETER_H

#include <string>
#include <vector>
#include "Variable.h"

namespace ost {
    class Semaphore;
}

namespace PdServ {
    class Parameter;
}

namespace MsrProto {

class Session;
class XmlElement;

class Parameter: public Variable {
    public:
        Parameter(const Server *,
                unsigned int parameterIndex,
                const PdServ::Parameter *p,
                unsigned int elementIndex = ~0U);

        void setXmlAttributes(XmlElement&, const char *buf,
                struct timespec const& ts, bool shortReply,
                bool hex, size_t precision,
                const std::string& id = std::string()) const;

        int setHexValue(const Session *,
                const char *str, size_t startindex, size_t &count) const;
        int setDoubleValue(const Session *,
                const char *, size_t startindex, size_t &count) const;

       void valueChanged(std::ostream &os, ost::Semaphore&,
               size_t start, size_t nelem) const;

       const PdServ::Parameter * const mainParam;

       typedef std::vector<const Parameter*> ChildList;
       const ChildList& getChildren() const;
       void addChild(const Parameter *);

    private:
        const bool dependent;
        const bool persistent;

       ChildList children;

        void (*append)(char *&, double);

        template<class T>
            static void setTo(char *&dst, double src);
};

}

#endif //MSRPARAMETER_H
