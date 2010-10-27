/*****************************************************************************
 * $Id$
 *
 * Copyright (C) 2010 Richard Hacker
 * License: GPL
 *
 *****************************************************************************/

#include "XmlDoc.h"

#include <iomanip>
#include <algorithm>
#include <cstring>

using namespace MsrXml;

/////////////////////////////////////////////////////////////////////////////
// Element
/////////////////////////////////////////////////////////////////////////////
Element::Element(const char *name, size_t indent):
    name(name), indent(indent)
{
}

/////////////////////////////////////////////////////////////////////////////
Element::~Element()
{
    for (Element::Children::const_iterator it = children.begin();
            it != children.end(); it++)
        delete *it;
}

/////////////////////////////////////////////////////////////////////////////
std::ostream& MsrXml::operator<<(std::ostream& os, const Element& el)
{
    os << std::string(el.indent, ' ') << '<' << el.name;
    for (Element::AttributeList::const_iterator it = el.attrList.begin();
            it != el.attrList.end(); it++)
        os << ' ' << *it << "=\"" << el.attrMap.find(*it)->second << '"';
    if (el.children.empty())
        os << "/>\r\n";
    else {
        os << ">\r\n";
        for (Element::Children::const_iterator it = el.children.begin();
                it != el.children.end(); it++)
            os << **it;
        os << std::string(el.indent, ' ') << "</" << el.name << ">\r\n";
    }
    return os;
}

/////////////////////////////////////////////////////////////////////////////
Element* Element::createChild(const char *name)
{
    Element *child;
    children.push_back(child = new Element(name, indent + 1));
    return child;
}

/////////////////////////////////////////////////////////////////////////////
void Element::appendChild(Element *child)
{
    if (child)
        children.push_back(child);
}

/////////////////////////////////////////////////////////////////////////////
void Element::releaseChildren()
{
    children.clear();
}

/////////////////////////////////////////////////////////////////////////////
bool Element::hasChildren() const
{
    return !children.empty();
}

/////////////////////////////////////////////////////////////////////////////
void Element::setAttribute(const char *a, const std::string &v)
{
    if (attrMap.find(a) == attrMap.end())
        attrList.push_back(a);

    attrMap[a] = v;
}

/////////////////////////////////////////////////////////////////////////////
void Element::setAttribute(const char *a, const char *v, size_t n)
{
    if (attrMap.find(a) == attrMap.end())
        attrList.push_back(a);

    attrMap[a] = n ? std::string(v,n) : v;
}


/////////////////////////////////////////////////////////////////////////////
void Element::setAttribute(const char *a, const struct timespec &t)
{
    if (attrMap.find(a) == attrMap.end())
        attrList.push_back(a);

    std::ostringstream os;
    os << t.tv_sec << '.' << std::setprecision(9) << t.tv_nsec;
    attrMap[a] = os.str();
}

/////////////////////////////////////////////////////////////////////////////
void Element::setAttributeCheck(const char *a, const std::string &v)
{
    setAttributeCheck(a, v.c_str(), v.size());
}

/////////////////////////////////////////////////////////////////////////////
void Element::setAttributeCheck(const char *a, const char *v, size_t n)
{
    if (attrMap.find(a) == attrMap.end())
        attrList.push_back(a);

    const char *escape = "<>&\"'";
    const char *escape_end = escape + 5;
    if (!n)
        n = strlen(v);
    const char *v_end = v + n;
    const char *p;
    std::string &s = attrMap[a];
    s = std::string();

    while ((p = std::find_first_of(v, v_end, escape, escape_end)) != v_end) {
        s += std::string(v, p - v);
        switch (*p) {
            case '<':
                s += "&lt;";
                break;

            case '>':
                s += "&gt;";
                break;

            case '&':
                s += "&amp;";
                break;

            case '"':
                s += "&quot;";
                break;

            case '\'':
                s += "&apos;";
                break;
        }

        v = p + 1;
    }
    s += std::string(v, v_end - v);
}