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

#include "XmlElement.h"
#include "../Variable.h"
#include "Variable.h"

#include <stdint.h>
#include <iomanip>
#include <algorithm>
#include <cstring>

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
// XmlElement
/////////////////////////////////////////////////////////////////////////////
XmlElement::XmlElement(const char *name): name(name)
{
}

/////////////////////////////////////////////////////////////////////////////
XmlElement::~XmlElement()
{
    // If any children exist, kill them too
    for (XmlElement::Children::const_iterator it = children.begin();
            it != children.end(); it++)
        delete *it;
}

/////////////////////////////////////////////////////////////////////////////
namespace MsrProto {
    std::ostream& operator<<(std::ostream& os, const XmlElement& el)
    {
        el.print(os);
        return os;
    }
}

/////////////////////////////////////////////////////////////////////////////
void XmlElement::print(std::ostream& os, size_t indent) const
{
    os << std::string(indent, ' ') << '<' << name;
    for (XmlElement::AttributeList::const_iterator it = attr.list.begin();
            it != attr.list.end(); it++)
        os << ' ' << *it->first << "=\"" << it->second << '"';
    if (children.empty())
        os << "/>\r\n";
    else {
        os << ">\r\n";
        for (XmlElement::Children::const_iterator it = children.begin();
                it != children.end(); it++)
            (*it)->print(os, indent+2);
        os << std::string(indent, ' ') << "</" << name << ">\r\n";
    }
}

/////////////////////////////////////////////////////////////////////////////
XmlElement* XmlElement::createChild(const char *name)
{
    XmlElement *child;
    children.push_back(child = new XmlElement(name));
    return child;
}

/////////////////////////////////////////////////////////////////////////////
void XmlElement::appendChild(XmlElement *child)
{
    if (child)
        children.push_back(child);
}

/////////////////////////////////////////////////////////////////////////////
void XmlElement::releaseChildren()
{
    children.clear();
}

/////////////////////////////////////////////////////////////////////////////
bool XmlElement::hasChildren() const
{
    return !children.empty();
}

/////////////////////////////////////////////////////////////////////////////
std::string& XmlElement::Attributes::operator[](const std::string &attr)
{
    AttributeMap::iterator it = find(attr);
    if (it == end()) {
        insert(std::make_pair(attr, reinterpret_cast<std::string*>(0)));
        it = find(attr);
        list.push_back(std::make_pair(&it->first, std::string()));
        it->second = &list.back().second;
    }
    return *it->second;
}

/////////////////////////////////////////////////////////////////////////////
void XmlElement::setAttribute(const char *a, const std::string &v)
{
    attr[a] = v;
}

/////////////////////////////////////////////////////////////////////////////
void XmlElement::setAttribute(const char *a, const char *v, size_t n)
{
    attr[a] = n ? std::string(v,n) : v;
}


/////////////////////////////////////////////////////////////////////////////
void XmlElement::setAttribute(const char *a, const struct timespec &t)
{
    std::ostringstream os;
    os << t.tv_sec << '.' << std::setprecision(6)
        << std::setw(6) << std::setfill('0') << t.tv_nsec / 1000;
    attr[a] = os.str();
}

/////////////////////////////////////////////////////////////////////////////
void XmlElement::setAttributeCheck(const char *a, const std::string &v)
{
    setAttributeCheck(a, v.c_str(), v.size());
}

/////////////////////////////////////////////////////////////////////////////
void XmlElement::setAttributeCheck(const char *a, const char *v, size_t n)
{
    const char *escape = "<>&\"'";
    const char *escape_end = escape + 5;
    const char *v_end = v + (n ? n : strlen(v));
    const char *p;
    std::string &s = attr[a];
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
