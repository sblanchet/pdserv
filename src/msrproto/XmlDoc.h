/*****************************************************************************
 * $Id$
 *
 * Copyright (C) 2010 Richard Hacker
 * License: GPL
 *
 * This is a minimal implementation of the XML dialect as used by
 * the MSR protocol.
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
        const char* data, size_t precision = 10, size_t n = 1);
std::string toHexDec( const HRTLab::Variable *v,
        const char* data, size_t precision = 10, size_t n = 1);
std::string toBase64( const HRTLab::Variable *v,
        const char* data, size_t precision = 10, size_t n = 1);

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
            void setAttribute(const char *name, const T& value);

        /** Special functions to set Parameter and Channel
         * attributes */
        void setParameterAttributes( const HRTLab::Parameter *p,
                bool shortReply, bool hex);
        void setChannelAttributes( const HRTLab::Signal *s,
                bool shortReply, double freq, size_t bufsize);

        /** Returns the data type of a variable */
        static const char *getDTypeName(const HRTLab::Variable*);

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
void Element::setAttribute(const char *name, const T& value)
{
    std::ostringstream o;
    o.imbue(std::locale::classic());

    o << value;
    setAttribute(name, o.str());
}

}

#endif // XMLDOC_H
