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
        //XmlElement(const char *name);
        XmlElement(const char *name, std::ostream &os);
        XmlElement(const char *name, XmlElement &parent);

        /** Destructor.
         * Children are destroyed if they were not released beforehand */
        ~XmlElement();

        std::ostream& prefix();

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
            void setAttribute(const char *name, const T& value);

        void csvAttribute(const char *name, const Variable *variable,
                const void *buf, size_t nblocks, size_t precision);
        void base64Attribute(const char *name,
                const void *data, size_t len) const;
        void hexDecAttribute(const char *name,
                const void *data, size_t len) const;

        /** Printing functions */
//        void print(std::ostream& os, size_t indent = 0) const;
//        friend std::ostream& operator<<(std::ostream& os, const XmlElement& el);


    private:
        std::ostream& os;

        const std::string name;
        bool printed;

//        typedef std::pair<const std::string *, std::string> AttributePair;
//        typedef std::list<AttributePair> AttributeList;
//        typedef std::map<std::string, std::string *> AttributeMap;
//        struct Attributes: private AttributeMap {
//            std::string& operator[](const std::string& attr);
//            AttributeList list;
//        };
//        Attributes attr;
//
//        typedef std::list<XmlElement*> Children;
//        Children children;
};

template <class T>
    void XmlElement::setAttribute(const char *name, const T& value)
    {
        os << ' ' << name << "=\"" << value << '"';
    }
}

#endif // XMLDOC_H
