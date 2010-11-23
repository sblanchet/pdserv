/*****************************************************************************
 * $Id$
 *
 * Copyright (C) 2010 Richard Hacker
 * License: GPL
 *
 *****************************************************************************/

#include "config.h"

#include "XmlDoc.h"
#include "../Variable.h"
#include "../Parameter.h"
#include "../Signal.h"

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
    os << t.tv_sec << '.' << std::setprecision(9)
        << std::setw(9) << std::setfill('0') << t.tv_nsec;
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
void Element::csvValueAttr(const char *attribute, const HRTLab::Variable *v,
        const char* data, size_t precision, size_t n)
{
    setAttribute(attribute, toCSV(v, data, precision, n));
}

/////////////////////////////////////////////////////////////////////////////
std::string MsrXml::toCSV(const HRTLab::Variable *v,
        const char* data, size_t precision, size_t n)
{
    std::ostringstream os;
    os.imbue(std::locale::classic());
    os.precision(precision);
    size_t count = v->nelem * n;

//    cout << "Eval si_double_T " << v->dtype << endl;
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
//            cout << "Eval si_double_T " << endl;
            os << reinterpret_cast<const double*>(data)[0];
            for ( size_t i = 1; i < count; i++)
                os << ',' << reinterpret_cast<const double*>(data)[i];
            break;
//            cout << "Eval si_double_T " << endl;

        default:
            break;
    }

    return os.str();
}

/////////////////////////////////////////////////////////////////////////////
void Element::hexDecValueAttr( const char *attr, const HRTLab::Variable *v,
        const char* data, size_t n)
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

    setAttribute(attr, s);
}

/////////////////////////////////////////////////////////////////////////////
void Element::base64ValueAttr(const char *attr, const HRTLab::Variable *v,
        const char* data, size_t precision, size_t n)
{
    // FIXME
}

/////////////////////////////////////////////////////////////////////////////
void Element::setParameterAttributes( const HRTLab::Parameter *p,
        const char *data, const struct timespec *mtime,
        bool shortReply, bool hex)
{
    // <parameter name="/lan/Control/EPC/EnableMotor/Value/2"
    //            index="30" value="0"/>

    setCommonAttributes(p, shortReply);

    if (!shortReply) {
        // flags= Add 0x100 for dependent variables
        setAttribute("flags", 3);
    }

    // mtime=
    setAttribute("mtime", *mtime);

    if (hex)
        hexDecValueAttr("hexvalue", p, data);
    else
        csvValueAttr("value", p, data);
    return;

    // persistent=
}

/////////////////////////////////////////////////////////////////////////////
void Element::setChannelAttributes( const HRTLab::Signal *s,
        const char *data, bool shortReply, double freq, size_t bufsize)
{
    // <channel name="/lan/World Time" alias="" index="0" typ="TDBL"
    //   datasize="8" bufsize="500" HZ="50" unit="" value="1283134199.93743"/>
    setCommonAttributes(s, shortReply);

    if (shortReply)
        return;

    // bufsize=
    setAttribute("bufsize", bufsize);
    setAttribute("HZ", freq);

    csvValueAttr("value", s, data);
}

/////////////////////////////////////////////////////////////////////////////
void Element::setCommonAttributes(const HRTLab::Variable *v, bool shortReply)
{
    // name=
    // value=
    // index=
    setAttribute("index", v->index);
    setAttributeCheck("name", v->path);

    if (shortReply)
        return;

    // datasize=
    // typ=
    setAttribute("alias", v->alias);
    setAttribute("datasize", v->width);
    // type=
    const char *dtype = 0;
    switch (v->dtype) {
        case si_boolean_T: dtype = "TCHAR";     break;
        case si_uint8_T:   dtype = "TUCHAR";    break;
        case si_sint8_T:   dtype = "TCHAR";     break;
        case si_uint16_T:  dtype = "TUSHORT";   break;
        case si_sint16_T:  dtype = "TSHORT";    break;
        case si_uint32_T:  dtype = "TUINT";     break;
        case si_sint32_T:  dtype = "TINT";      break;
        case si_uint64_T:  dtype = "TULINT";    break;
        case si_sint64_T:  dtype = "TLINT";     break;
        case si_single_T:  dtype = "TDBL";      break;
        case si_double_T:  dtype = "TFLT";      break;
        default:                                break;
    }
    setAttribute("typ", dtype);

    // For vectors:
    // anz=
    // cnum=
    // rnum=
    // orientation=
    if (v->nelem > 1) {
        setAttribute("anz",v->nelem);
        const char *orientation;
        size_t cnum, rnum;
        switch (v->ndims) {
            case 1:
                {
                    cnum = v->nelem;
                    rnum = 1;
                    orientation = "VECTOR";
                }
                break;

            case 2:
                {
                    const size_t *dim = v->getDim();
                    cnum = dim[1];
                    rnum = dim[0];
                    orientation = "MATRIX_ROW_MAJOR";
                }
                break;

            default:
                {
                    const size_t *dim = v->getDim();
                    cnum = dim[v->ndims - 1];
                    rnum = v->nelem / cnum;
                    orientation = "MATRIX_ROW_MAJOR";
                }
                break;
        }
        setAttribute("cnum", cnum);
        setAttribute("rnum", rnum);
        setAttribute("orientation", orientation);
    }

    // unit=
    if (v->unit.size())
        setAttributeCheck("unit", v->unit);

    // text=
    if (v->comment.size())
        setAttributeCheck("text", v->comment);

    // hide=
    // unhide=
}
