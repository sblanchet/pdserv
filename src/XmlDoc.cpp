#include <sstream>
#include <iomanip>
#include "XmlDoc.h"

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
void Element::setAttribute(const char *a, const char *v, size_t n)
{
    // FIXME: Check for escaped characters
    attr[a] = n ? std::string(v, n) : v;
}

/////////////////////////////////////////////////////////////////////////////
void Element::setAttribute(const char *a, const struct timespec &t)
{
    std::ostringstream os;
    os << t.tv_sec << '.' << std::setprecision(9) << t.tv_nsec;
    attr[a] = os.str();
}

/////////////////////////////////////////////////////////////////////////////
void Element::setAttribute(const char *a, const std::string &v)
{
    // FIXME: Check for escaped characters
    attr[a] = v;
}

/////////////////////////////////////////////////////////////////////////////
std::ostream& MsrXml::operator<<(std::ostream& os, const Element& el)
{
    os << std::string(el.indent, ' ') << '<' << el.name;
    for (Element::Attribute::const_iterator it = el.attr.begin();
            it != el.attr.end(); it++)
        os << ' ' << it->first << "=\"" << it->second << '"';
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

