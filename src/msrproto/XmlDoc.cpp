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

#include "config.h"

#include "XmlDoc.h"
#include "../Variable.h"

#include <stdint.h>
#include <iomanip>
#include <algorithm>
#include <cstring>

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace MsrXml;

/////////////////////////////////////////////////////////////////////////////
// Element
/////////////////////////////////////////////////////////////////////////////
Element::Element(const char *name): name(name)
{
}

/////////////////////////////////////////////////////////////////////////////
Element::~Element()
{
    // If any children exist, kill them too
    for (Element::Children::const_iterator it = children.begin();
            it != children.end(); it++)
        delete *it;
}

/////////////////////////////////////////////////////////////////////////////
std::ostream& MsrXml::operator<<(std::ostream& os, const Element& el)
{
    el.print(os);
    return os;
}

/////////////////////////////////////////////////////////////////////////////
void Element::print(std::ostream& os, size_t indent) const
{
    os << std::string(indent, ' ') << '<' << name;
    for (Element::AttributeList::const_iterator it = attrList.begin();
            it != attrList.end(); it++)
        os << ' ' << *it << "=\"" << attrMap.find(*it)->second << '"';
    if (children.empty())
        os << "/>\r\n";
    else {
        os << ">\r\n";
        for (Element::Children::const_iterator it = children.begin();
                it != children.end(); it++)
            (*it)->print(os, indent+2);
        os << std::string(indent, ' ') << "</" << name << ">\r\n";
    }
}

/////////////////////////////////////////////////////////////////////////////
Element* Element::createChild(const char *name)
{
    Element *child;
    children.push_back(child = new Element(name));
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
    os << t.tv_sec << '.' << std::setprecision(6)
        << std::setw(6) << std::setfill('0') << t.tv_nsec / 1000;
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
    const char *v_end = v + (n ? n : strlen(v));
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

