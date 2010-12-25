/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdcomserv package.
 *
 *  pdcomserv is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pdcomserv is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pdcomserv. See COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include "config.h"

#include "PrintVariable.h"
#include "XmlDoc.h"

#include "../Variable.h"

#include <stdint.h>
//#include <iomanip>
//#include <algorithm>
//#include <cstring>
#include <cstdio>

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
// PrintFunc
/////////////////////////////////////////////////////////////////////////////
template<class T>
void print(std::ostringstream& os,
        const char *data, size_t n, unsigned int)
{
    os << reinterpret_cast<const T*>(data)[n];
}

/////////////////////////////////////////////////////////////////////////////
template<>
void print<double>(std::ostringstream& os,
        const char *data, size_t n, unsigned int precision)
{
    char buf[precision + 10];

    snprintf(buf, sizeof(buf), "%.*g", precision, ((double*)data)[n]);

    os << buf;
}

/////////////////////////////////////////////////////////////////////////////
template<>
void print<float>(std::ostringstream& os,
        const char *data, size_t n, unsigned int precision)
{
    char buf[precision + 10];

    snprintf(buf, sizeof(buf), "%.*g", precision, ((float*)data)[n]);

    os << buf;
}

/////////////////////////////////////////////////////////////////////////////
PrintFunc MsrProto::getPrintFunc(enum si_datatype_t dtype)
{
    switch (dtype) {
        case si_boolean_T:  return print<bool>;
        case si_uint8_T:    return print<uint8_t>;
        case si_sint8_T:    return print<int8_t>;
        case si_uint16_T:   return print<uint16_t>;
        case si_sint16_T:   return print<int16_t>;
        case si_uint32_T:   return print<uint32_t>;
        case si_sint32_T:   return print<int32_t>;
        case si_uint64_T:   return print<uint64_t>;
        case si_sint64_T:   return print<int64_t>;
        case si_double_T:   return print<double>;
        case si_single_T:   return print<float>;
        default         :   return 0;
    }
}

/////////////////////////////////////////////////////////////////////////////
void MsrProto::csvAttribute( MsrXml::Element *element, const char *name,
        PrintFunc printFunc, const HRTLab::Variable *v,
        size_t count, const char* data, size_t precision)
{
    element->setAttribute(name, toCSV(printFunc, v, count, data, precision));
}

/////////////////////////////////////////////////////////////////////////////
std::string MsrProto::toCSV( PrintFunc printfunc, const HRTLab::Variable *v,
        size_t count, const char* data, size_t precision)
{
    std::ostringstream os;
    os.imbue(std::locale::classic());

    for ( size_t i = 0; i < count; i++) {
        if (i) os << ',';
        printfunc(os, data, i, precision);
    }

    return os.str();
}

/////////////////////////////////////////////////////////////////////////////
void MsrProto::hexDecAttribute( MsrXml::Element *element, const char *name,
        const HRTLab::Variable *v, size_t count, const char *data)
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

    size_t charCount = v->width * count * 2;

    s.reserve(charCount + 1);

    for (size_t i = 0; i < charCount; i++)
        s.append(hexValue[static_cast<unsigned char>(data[i])], 2);

    element->setAttribute(name, s);
}

/////////////////////////////////////////////////////////////////////////////
void MsrProto::base64Attribute( MsrXml::Element *element, const char *name,
        const HRTLab::Variable *v, size_t count, const char *data)
{
    static const char *base64Chr = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz0123456789+/";
    count *= v->memSize;
    size_t len = (count + 2 - ((count + 2) % 3)) / 3 * 4;
    char s[len + 1];
    char *p = s;
    size_t i = 0;
    size_t rem = count % 3;
    const unsigned char *udata = reinterpret_cast<const unsigned char*>(data);

    // First convert all characters in chunks of 3
    while (i != count - rem) {
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

    element->setAttribute(name, s);
}

/////////////////////////////////////////////////////////////////////////////
void MsrProto::setVariableAttributes(
        MsrXml::Element *element,
        const HRTLab::Variable *v,
        unsigned int index,
        const std::string& path,
        size_t nelem,
        bool shortReply)
{
    // name=
    // value=
    // index=
    element->setAttribute("index", index);
    element->setAttributeCheck("name", path);

    if (shortReply)
        return;

    // alias=
    // unit=
    // comment=
    if (!v->alias.empty())
        element->setAttributeCheck("alias", v->alias);
    if (!v->unit.empty())
        element->setAttributeCheck("unit", v->unit);
    if (!v->comment.empty())
        element->setAttributeCheck("comment", v->comment);

    // datasize=
    element->setAttribute("datasize", v->width);

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
    element->setAttribute("typ", dtype);

    // For vectors:
    // anz=
    // cnum=
    // rnum=
    // orientation=
    if (nelem > 1) {
        element->setAttribute("anz",nelem);
        const char *orientation;
        size_t cnum, rnum;
        if (v->nelem != nelem) {
            // Traditional version where an n-dim array is split into a set
            // of individual vectors
            orientation = "VECTOR";
            rnum = 1;
            cnum = nelem;
        }
        else {
            // Transmit either a vector or a matrix
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
        }
        element->setAttribute("cnum", cnum);
        element->setAttribute("rnum", rnum);
        element->setAttribute("orientation", orientation);
    }

    // unit=
    if (!v->unit.empty())
        element->setAttributeCheck("unit", v->unit);

    // text=
    if (!v->comment.empty())
        element->setAttributeCheck("text", v->comment);

    // hide=
    // unhide=
}
