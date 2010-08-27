#include <stdint.h>
#include <sstream>
#include <iomanip>
#include "XmlDoc.h"
#include "Variable.h"

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
    // FIXME: Check for escaped characters
    attr[a] = os.str();
}

/////////////////////////////////////////////////////////////////////////////
void Element::setAttribute(const char *a, const std::string &v)
{
    // FIXME: Check for escaped characters
    attr[a] = v;
}

/////////////////////////////////////////////////////////////////////////////
void Element::setAttribute(const char *a,
        const HRTLab::Variable *v, const char* data)
{
    std::ostringstream os;

    switch (v->dtype) {
        case si_boolean_T:
            {
                for ( size_t i = 0; i < v->nelem; i++) {
                    if (i)
                        os << ',';
                    os << reinterpret_cast<const bool*>(data)[i];
                }
            }
            break;

        case si_uint8_T:
            {
                for ( size_t i = 0; i < v->nelem; i++) {
                    if (i)
                        os << ',';
                    os << reinterpret_cast<const uint8_t*>(data)[i];
                }
            }
            break;

        case si_sint8_T:
            {
                for ( size_t i = 0; i < v->nelem; i++) {
                    if (i)
                        os << ',';
                    os << reinterpret_cast<const int8_t*>(data)[i];
                }
            }
            break;

        case si_uint16_T:
            {
                for ( size_t i = 0; i < v->nelem; i++) {
                    if (i)
                        os << ',';
                    os << reinterpret_cast<const uint16_t*>(data)[i];
                }
            }
            break;

        case si_sint16_T:
            {
                for ( size_t i = 0; i < v->nelem; i++) {
                    if (i)
                        os << ',';
                    os << reinterpret_cast<const int16_t*>(data)[i];
                }
            }
            break;

        case si_uint32_T:
            {
                for ( size_t i = 0; i < v->nelem; i++) {
                    if (i)
                        os << ',';
                    os << reinterpret_cast<const uint32_t*>(data)[i];
                }
            }
            break;

        case si_sint32_T:
            {
                for ( size_t i = 0; i < v->nelem; i++) {
                    if (i)
                        os << ',';
                    os << reinterpret_cast<const int32_t*>(data)[i];
                }
            }
            break;

        case si_uint64_T:
            {
                for ( size_t i = 0; i < v->nelem; i++) {
                    if (i)
                        os << ',';
                    os << reinterpret_cast<const uint64_t*>(data)[i];
                }
            }
            break;

        case si_sint64_T:
            {
                for ( size_t i = 0; i < v->nelem; i++) {
                    if (i)
                        os << ',';
                    os << reinterpret_cast<const int64_t*>(data)[i];
                }
            }
            break;

        case si_single_T:
            {
                for ( size_t i = 0; i < v->nelem; i++) {
                    if (i)
                        os << ',';
                    os << reinterpret_cast<const float*>(data)[i];
                }
            }
            break;

        case si_double_T:
            {
                for ( size_t i = 0; i < v->nelem; i++) {
                    if (i)
                        os << ',';
                    os << reinterpret_cast<const double*>(data)[i];
                }
            }
            break;

        default:
            break;
    }

    // FIXME: Check for escaped characters
    attr[a] = os.str();
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

