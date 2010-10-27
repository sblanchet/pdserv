/*****************************************************************************
 * $Id$
 *
 * Copyright (C) 2010 Richard Hacker
 * License: GPL
 *
 *****************************************************************************/

#include "XmlDoc.h"
#include "../Variable.h"

#include <stdint.h>
#include <iomanip>
#include <algorithm>
#include <cstring>

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

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

/////////////////////////////////////////////////////////////////////////////
std::string MsrXml::toCSV( const HRTLab::Variable *v,
        const char* data, size_t precision, size_t n)
{
    std::ostringstream os;
    os.imbue(std::locale::classic());
    os.precision(precision);
    size_t count = v->nelem * n;

    cout << "Eval si_double_T " << v->dtype << endl;
    switch (v->dtype) {
        case si_boolean_T:
            os << reinterpret_cast<const bool*>(data)[0];
            for ( size_t i = 1; i < count; i++)
                os << ',' << reinterpret_cast<const bool*>(data)[i];
            break;

        case si_uint8_T:
            os << reinterpret_cast<const uint8_t*>(data)[0];
            for ( size_t i = 1; i < count; i++)
                os << ',' << reinterpret_cast<const uint8_t*>(data)[i];
            break;

        case si_sint8_T:
            os << reinterpret_cast<const int8_t*>(data)[0];
            for ( size_t i = 1; i < count; i++)
                os << ',' << reinterpret_cast<const int8_t*>(data)[i];
            break;

        case si_uint16_T:
            os << reinterpret_cast<const uint16_t*>(data)[0];
            for ( size_t i = 1; i < count; i++)
                os << ',' << reinterpret_cast<const uint16_t*>(data)[i];
            break;

        case si_sint16_T:
            os << reinterpret_cast<const int16_t*>(data)[0];
            for ( size_t i = 1; i < count; i++)
                os << ',' << reinterpret_cast<const int16_t*>(data)[i];
            break;

        case si_uint32_T:
            os << reinterpret_cast<const uint32_t*>(data)[0];
            for ( size_t i = 1; i < count; i++)
                os << ',' << reinterpret_cast<const uint32_t*>(data)[i];
            break;

        case si_sint32_T:
            os << reinterpret_cast<const int32_t*>(data)[0];
            for ( size_t i = 1; i < count; i++)
                os << ',' << reinterpret_cast<const int32_t*>(data)[i];
            break;

        case si_uint64_T:
            os << reinterpret_cast<const uint64_t*>(data)[0];
            for ( size_t i = 1; i < count; i++)
                os << ',' << reinterpret_cast<const uint64_t*>(data)[i];
            break;

        case si_sint64_T:
            os << reinterpret_cast<const int64_t*>(data)[0];
            for ( size_t i = 1; i < count; i++)
                os << ',' << reinterpret_cast<const int64_t*>(data)[i];
            break;

        case si_single_T:
            os << reinterpret_cast<const float*>(data)[0];
            for ( size_t i = 1; i < count; i++)
                os << ',' << reinterpret_cast<const float*>(data)[i];
            break;

        case si_double_T:
            cout << "Eval si_double_T " << endl;
            os << reinterpret_cast<const double*>(data)[0];
            for ( size_t i = 1; i < count; i++)
                os << ',' << reinterpret_cast<const double*>(data)[i];
            break;
            cout << "Eval si_double_T " << endl;

        default:
            break;
    }

    return os.str();
}

/////////////////////////////////////////////////////////////////////////////
std::string MsrXml::toHexDec( const HRTLab::Variable *v,
        const char* data, size_t precision, size_t n)
{
    std::string s;
    const char *hexValue[256] = {
        "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", 
        "0A", "0B", "0C", "0D", "0E", "0F", "10", "11", "12", "13", 
        "14", "15", "16", "17", "18", "19", "1A", "1B", "1C", "1D", 
        "1E", "1F", "20", "21", "22", "23", "24", "25", "26", "27", 
        "28", "29", "2A", "2B", "2C", "2D", "2E", "2F", "30", "31", 
        "32", "33", "34", "35", "36", "37", "38", "39", "3A", "3B", 
        "3C", "3D", "3E", "3F", "40", "41", "42", "43", "44", "45", 
        "46", "47", "48", "49", "4A", "4B", "4C", "4D", "4E", "4F", 
        "50", "51", "52", "53", "54", "55", "56", "57", "58", "59", 
        "5A", "5B", "5C", "5D", "5E", "5F", "60", "61", "62", "63", 
        "64", "65", "66", "67", "68", "69", "6A", "6B", "6C", "6D", 
        "6E", "6F", "70", "71", "72", "73", "74", "75", "76", "77", 
        "78", "79", "7A", "7B", "7C", "7D", "7E", "7F", "80", "81", 
        "82", "83", "84", "85", "86", "87", "88", "89", "8A", "8B", 
        "8C", "8D", "8E", "8F", "90", "91", "92", "93", "94", "95", 
        "96", "97", "98", "99", "9A", "9B", "9C", "9D", "9E", "9F", 
        "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8", "A9", 
        "AA", "AB", "AC", "AD", "AE", "AF", "B0", "B1", "B2", "B3", 
        "B4", "B5", "B6", "B7", "B8", "B9", "BA", "BB", "BC", "BD", 
        "BE", "BF", "C0", "C1", "C2", "C3", "C4", "C5", "C6", "C7", 
        "C8", "C9", "CA", "CB", "CC", "CD", "CE", "CF", "D0", "D1", 
        "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "DA", "DB", 
        "DC", "DD", "DE", "DF", "E0", "E1", "E2", "E3", "E4", "E5", 
        "E6", "E7", "E8", "E9", "EA", "EB", "EC", "ED", "EE", "EF", 
        "F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", 
        "FA", "FB", "FC", "FD", "FE", "FF"};

    s.reserve(v->memSize*2 + 1);

    for (size_t i = 0; i < v->memSize * n; i++)
        s.append(hexValue[static_cast<unsigned char>(data[i])], 2);

    return s;
}

/////////////////////////////////////////////////////////////////////////////
std::string MsrXml::toBase64( const HRTLab::Variable *v,
        const char* data, size_t precision, size_t n)
{
    return "";
}

