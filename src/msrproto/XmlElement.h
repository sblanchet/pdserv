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
class Parameter;
class Channel;

class XmlElement {
    public:
        //XmlElement(const char *name);
        XmlElement(const char *name, std::ostream *os);
        XmlElement(const char *name, XmlElement &parent);

        /** Destructor.
         * Children are destroyed if they were not released beforehand */
        ~XmlElement();

        std::ostream& prefix();

        void setAttributes(const Parameter *p,
                const char *buf, struct timespec const&,
                bool shortReply, bool hex, bool writeAccess,
                size_t precision, const std::string& id = std::string());
        void setAttributes(const Channel *c, const char *buf,
                bool shortReply, size_t precision);

        class Attribute {
            public:
                Attribute(XmlElement&, const char *name);
                ~Attribute();

                /** Set string attribute, checking for characters
                 * that need to be escaped */
                void setWithCare( const char *value, size_t n = 0);

                void csv(const Variable* var, const char *buf,
                        size_t nblocks, size_t precision);
                void base64( const void *data, size_t len) const;
                void hexDec( const void *data, size_t len) const;


                std::ostream& operator<<(const struct timespec &t);

                /** Various variations of numerical attributes */
                std::ostream& operator<<(const char *s);
                template <typename T>
                    std::ostream& operator<<(T const& val);

            private:
                std::ostream& os;
                
        };

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

    template <typename T>
std::ostream& XmlElement::Attribute::operator<<(T const& val)
{
    os << val;
    return os;
}

}

#endif // XMLDOC_H
