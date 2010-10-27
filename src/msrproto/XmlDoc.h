/*****************************************************************************
 * $Id$
 *
 * Copyright (C) 2010 Richard Hacker
 * License: GPL
 *
 * This is a minimal implementation of the XML dialect as used by
 * the MSR protocol.
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
}

namespace MsrXml {

class Element {
    public:
        Element(const char *name, size_t indent = 0);

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
            void setAttribute(const char *name, const T& value);

        friend std::ostream& operator<<(std::ostream& os, const Element& el);

    private:
        const std::string name;
        const size_t indent;    // Indent used when printing

        typedef std::map<const std::string, std::string> AttributeMap;
        AttributeMap attrMap;

        typedef std::list<std::string> AttributeList;
        AttributeList attrList;

        typedef std::list<Element*> Children;
        Children children;
};

std::ostream& operator<<(std::ostream& os, const Element& el);

template <class T>
void Element::setAttribute(const char *name, const T& value)
{
    std::ostringstream o;
    o.imbue(std::locale::classic());

    o << value;
    if (attrMap.find(name) == attrMap.end())
        attrList.push_back(name);

    attrMap[name] = o.str();
}

}

#endif // XMLDOC_H
