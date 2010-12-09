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
#include "../Task.h"

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

/////////////////////////////////////////////////////////////////////////////
void Element::csvValueAttr(const char *attribute, const HRTLab::Variable *v,
        const char* data, size_t precision, size_t n)
{
    setAttribute(attribute, toCSV(v, data, precision, n));
}

/////////////////////////////////////////////////////////////////////////////
template<class T>
void print(std::ostringstream& os,
        const char *data, size_t n)
{
    os << reinterpret_cast<const T*>(data)[n];
}

/////////////////////////////////////////////////////////////////////////////
std::string MsrXml::toCSV(const HRTLab::Variable *v,
        const char* data, size_t precision, size_t n)
{
    std::ostringstream os;
    os.imbue(std::locale::classic());
    os.precision(precision);
    size_t count = v->nelem * n;

    void (*printfunc)(std::ostringstream& os,
        const char *data, size_t n);

    switch (v->dtype) {
        case si_boolean_T:  printfunc = print<bool>;           break;
        case si_uint8_T:    printfunc = print<uint8_t>;        break;
        case si_sint8_T:    printfunc = print<int8_t>;         break;
        case si_uint16_T:   printfunc = print<uint16_t>;       break;
        case si_sint16_T:   printfunc = print<int16_t>;        break;
        case si_uint32_T:   printfunc = print<uint32_t>;       break;
        case si_sint32_T:   printfunc = print<int32_t>;        break;
        case si_uint64_T:   printfunc = print<uint64_t>;       break;
        case si_sint64_T:   printfunc = print<int64_t>;        break;
        case si_double_T:   printfunc = print<double>;         break;
        case si_single_T:   printfunc = print<float>;          break;
        default:            printfunc = 0;                     break;
    }

    for ( size_t i = 0; i < count; i++) {
        if (i) os << ',';
        printfunc(os, data, i);
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
    static const char *base64Chr = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz0123456789+/";
    n *= v->memSize;
    size_t len = (n + 2 - ((n + 2) % 3)) / 3 * 4;
    char s[len + 1];
    char *p = s;
    size_t i = 0;
    size_t rem = n % 3;
    const unsigned char *udata = reinterpret_cast<const unsigned char*>(data);

    // First convert all characters in chunks of 3
    while (i != n - rem) {
        *p++ = base64Chr[  udata[i  ]         >> 2];
        *p++ = base64Chr[((udata[i  ] & 0x03) << 4) + (udata[i+1] >> 4)];
        *p++ = base64Chr[((udata[i+1] & 0x0f) << 2) + (udata[i+2] >> 6)];
        *p++ = base64Chr[ (udata[i+2] & 0x3f)     ];

        i += 3;
    }

    // Convert the remaining 1 or 2 characters
    switch (rem) {
        case 2:
            *p++ = base64Chr[  udata[i  ]         >> 2];
            *p++ = base64Chr[((udata[i  ] & 0x03) << 4) + (udata[i+1] >> 4)];
            *p++ = base64Chr[ (udata[i+1] & 0x0f) << 2];
            break;
        case 1:
            *p++ = base64Chr[  udata[i]         >> 2];
            *p++ = base64Chr[ (udata[i] & 0x03) << 4];
            break;
    }

    // Fill the remaining space with '='
    while ( (p - s) % 4)
        *p++ = '=';

    // Terminate the string
    *p = '\0';

    setAttribute(attr, s);
}

/////////////////////////////////////////////////////////////////////////////
void Element::setParameterAttributes( const HRTLab::Parameter *p,
        unsigned int index, unsigned int flags, bool shortReply, bool hex)
{
    struct timespec mtime;
    char data[p->memSize];

    p->getValue(data, &mtime);

    // <parameter name="/lan/Control/EPC/EnableMotor/Value/2"
    //            index="30" value="0"/>

    setCommonAttributes(p, index, shortReply);

    if (!shortReply) {
        // flags= Add 0x100 for dependent variables
        // FIX
        setAttribute("flags", flags,
                std::ios::hex | std::ios::showbase);
    }

    // mtime=
    setAttribute("mtime", mtime);

    if (hex)
        hexDecValueAttr("hexvalue", p, data);
    else
        csvValueAttr("value", p, data);
    return;

    // persistent=
}

/////////////////////////////////////////////////////////////////////////////
void Element::setChannelAttributes( const HRTLab::Signal *s,
        unsigned int index, bool shortReply, const char *data)
{
    // <channel name="/lan/World Time" alias="" index="0" typ="TDBL"
    //   datasize="8" bufsize="500" HZ="50" unit="" value="1283134199.93743"/>
    double freq = 1.0 / s->task->sampleTime / s->decimation;

    // The MSR protocoll wants a bufsize, the maximum number of
    // values that can be retraced. This artificial limitation does
    // not exist any more. Instead, choose a buffer size so that
    // at a maximum of 1 second has to be stored.
    size_t bufsize = std::max( 1U, (size_t)(freq + 0.5));


    setCommonAttributes(s, index, shortReply);

    if (shortReply)
        return;

    // bufsize=
    setAttribute("bufsize", bufsize);
    setAttribute("HZ", freq);

    csvValueAttr("value", s, data);
}

/////////////////////////////////////////////////////////////////////////////
void Element::setCommonAttributes(const HRTLab::Variable *v,
        unsigned int index, bool shortReply)
{
    // name=
    // value=
    // index=
    setAttribute("index", index);
    setAttributeCheck("name", v->path);

    if (shortReply)
        return;

    // alias=
    // unit=
    // comment=
    if (!v->alias.empty())
        setAttributeCheck("alias", v->alias);
    if (!v->unit.empty())
        setAttributeCheck("unit", v->unit);
    if (!v->comment.empty())
        setAttributeCheck("comment", v->comment);

    // datasize=
    setAttribute("datasize", v->width);

    // typ=
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
