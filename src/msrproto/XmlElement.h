/*****************************************************************************
 *
 *  $Id$
 *
 *  This is a minimal implementation of the XML dialect as used by
 *  the MSR protocol.
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

#ifndef XMLDOC_H
#define XMLDOC_H

#include <sstream>
#include <string>
#include <map>
#include <list>
//#include "pdserv/etl_data_info.h"

namespace PdServ {
    class Variable;
}

namespace MsrProto {

class Variable;

class XmlElement {
    public:
        XmlElement(const char *name);

        /** Destructor.
         * Children are destroyed if they were not released beforehand */
        ~XmlElement();

        /** Create an element child */
        XmlElement* createChild(const char *name);

        /** Append a child to the current element */
        void appendChild(XmlElement *);

        /** Release children.
         * The children will then not be destroyed when the parent is killed
         */
        void releaseChildren();

        /** Return true if the node has children */
        bool hasChildren() const;

        /** Set string attribute, checking for characters that need to be
         * escaped */
        void setAttributeCheck(const char *name,
                const char *value, size_t n = 0);
        void setAttributeCheck(const char *name, const std::string& value);

        /** Set verbatim string attribute */
        void setAttribute(const char *name, const char *value, size_t n = 0);
        void setAttribute(const char *name, const std::string& value);

        /** Various variations of numerical attributes */
        void setAttribute(const char *name, const struct timespec&);
        template<class T>
            void setAttribute(const char *name, const T& value,
                    std::ios::fmtflags flags = std::ios::dec);

        /** Printing functions */
        void print(std::ostream& os, size_t indent = 0) const;
        friend std::ostream& operator<<(std::ostream& os, const XmlElement& el);


    private:
        const std::string name;

        typedef std::pair<const std::string *, std::string> AttributePair;
        typedef std::list<AttributePair> AttributeList;
        typedef std::map<std::string, std::string *> AttributeMap;
        struct Attributes: private AttributeMap {
            std::string& operator[](const std::string& attr);
            AttributeList list;
        };
        Attributes attr;

        typedef std::list<XmlElement*> Children;
        Children children;
};

template <class T>
    void XmlElement::setAttribute(const char *name, const T& value,
            std::ios::fmtflags flags)
    {
        std::ostringstream os;
        os.imbue(std::locale::classic());
        os.setf(flags, std::ios_base::basefield);
        os.setf(flags);

        os << value;
        setAttribute(name, os.str());
    }
}

#endif // XMLDOC_H
