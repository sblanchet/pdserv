/*****************************************************************************
 *
 *  $Id$
 *
 *  This is a minimal implementation of the XML dialect as used by
 *  the MSR protocol.
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

#ifndef XMLDOC_H
#define XMLDOC_H

#include <sstream>
#include <string>
#include <map>
#include <list>
#include <ctime>

namespace HRTLab {
    class Variable;
    class Signal;
    class Parameter;
}

namespace MsrXml {

class Element;

std::string toCSV( const HRTLab::Variable *v,
        const char *data, size_t precision = 10, size_t n = 1);

class Element {
    public:
        Element(const char *name);

        /** Destructor.
         * Children are destroyed if they were not released beforehand */
        ~Element();

        /** Create an element child */
        Element* createChild(const char *name);

        /** Append a child to the current element */
        void appendChild(Element *);

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

        void base64ValueAttr( const char *name, const HRTLab::Variable *v,
                const char* data, size_t precision = 10, size_t n = 1);
        void csvValueAttr(    const char *name, const HRTLab::Variable *v,
                const char* data, size_t precision = 10, size_t n = 1);
        void hexDecValueAttr(  const char *name, const HRTLab::Variable *v,
                const char* data, size_t n = 1);

        /** Special functions to set Parameter and Channel
         * attributes */
        void setCommonAttributes(const HRTLab::Variable*,
                unsigned int index, bool shortReply);
        void setParameterAttributes( const HRTLab::Parameter *p,
                unsigned int index, unsigned int flags, bool shortReply,
                bool hex);
        void setChannelAttributes( const HRTLab::Signal *s,
                unsigned int index, bool shortReply, const char *data);

        /** Printing functions */
        void print(std::ostream& os, size_t indent = 0) const;
        friend std::ostream& operator<<(std::ostream& os, const Element& el);

    private:
        const std::string name;

        typedef std::map<const std::string, std::string> AttributeMap;
        AttributeMap attrMap;

        typedef std::list<std::string> AttributeList;
        AttributeList attrList;

        typedef std::list<Element*> Children;
        Children children;
};

std::ostream& operator<<(std::ostream& os, const Element& el);

template <class T>
void Element::setAttribute(const char *name, const T& value,
        std::ios::fmtflags flags)
{
    std::ostringstream o;
    o.imbue(std::locale::classic());
    o.setf(flags, std::ios_base::basefield);
    o.setf(flags);

    o << value;
    setAttribute(name, o.str());
}

}

#endif // XMLDOC_H
