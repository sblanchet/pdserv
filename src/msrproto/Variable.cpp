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

#include "Variable.h"
#include "XmlElement.h"
#include "Directory.h"
#include "../Signal.h"
#include "../Parameter.h"

#include <cstdio>
#include <stdint.h>
#include <sstream>

using namespace MsrProto;

namespace MsrProto {

    /////////////////////////////////////////////////////////////////////////
    template <class T>
        void Variable::print(std::ostream &os,
                const void* data, size_t n, unsigned int)
        {
            os << reinterpret_cast<const T*>(data)[n];
        }

    /////////////////////////////////////////////////////////////////////////
    template<>
        void Variable::print<int8_t>(std::ostream& os,
                const void *data, size_t n, unsigned int)
        {
            os << (int)reinterpret_cast<const int8_t*>(data)[n];
        }

    /////////////////////////////////////////////////////////////////////////
    template<>
        void Variable::print<uint8_t>(std::ostream& os,
                const void *data, size_t n, unsigned int)
        {
            os << (unsigned int)reinterpret_cast<const uint8_t*>(data)[n];
        }

    /////////////////////////////////////////////////////////////////////////
    template<>
        void Variable::print<double>(std::ostream& os,
                const void *data, size_t n, unsigned int precision)
        {
            char buf[precision + 10];

            snprintf(buf, sizeof(buf), "%.*g", precision,
                    reinterpret_cast<const double*>(data)[n]);

            os << buf;
        }

    /////////////////////////////////////////////////////////////////////////
    template<>
        void Variable::print<float>(std::ostream& os,
                const void *data, size_t n, unsigned int precision)
        {
            char buf[precision + 10];

            snprintf(buf, sizeof(buf), "%.*g", precision,
                    reinterpret_cast<const float*>(data)[n]);

            os << buf;
        }
}

/////////////////////////////////////////////////////////////////////////////
Variable::Variable( const DirectoryNode *directory,
        const PdServ::Variable *v, unsigned int variableIndex,
        unsigned int elementIndex):
    directory(directory),
    elementIndex(elementIndex != ~0U ? elementIndex : 0),
    variable(v),
    variableIndex(variableIndex),
    nelem(elementIndex == ~0U ? v->nelem : 1), memSize(nelem * v->width)
//    bufferOffset(elementIndex == ~0U ? 0 : elementIndex * v->width)
{
    switch (v->dtype) {
        case PdServ::Variable::boolean_T: printFunc = print<bool>;      break;
        case PdServ::Variable::uint8_T:   printFunc = print<uint8_t>;   break;
        case PdServ::Variable::int8_T:    printFunc = print<int8_t>;    break;
        case PdServ::Variable::uint16_T:  printFunc = print<uint16_t>;  break;
        case PdServ::Variable::int16_T:   printFunc = print<int16_t>;   break;
        case PdServ::Variable::uint32_T:  printFunc = print<uint32_t>;  break;
        case PdServ::Variable::int32_T:   printFunc = print<int32_t>;   break;
        case PdServ::Variable::uint64_T:  printFunc = print<uint64_t>;  break;
        case PdServ::Variable::int64_T:   printFunc = print<int64_t>;   break;
        case PdServ::Variable::double_T:  printFunc = print<double>;    break;
        case PdServ::Variable::single_T:  printFunc = print<float>;     break;
    }
}

/////////////////////////////////////////////////////////////////////////////
Variable::~Variable()
{
}

/////////////////////////////////////////////////////////////////////////////
std::string Variable::path() const
{
    return directory->path();
}

/////////////////////////////////////////////////////////////////////////////
const PdServ::Signal* Variable::signal() const
{
    return dynamic_cast<const PdServ::Signal*>(variable);
}

/////////////////////////////////////////////////////////////////////////////
const PdServ::Parameter* Variable::parameter() const
{
    return dynamic_cast<const PdServ::Parameter*>(variable);
}

/////////////////////////////////////////////////////////////////////////////
void Variable::setAttributes(XmlElement *element, bool shortReply) const
{
     // name=
     // value=
     // index=
     element->setAttribute("index", variableIndex);
     element->setAttributeCheck("name", path());
 
     if (shortReply)
         return;
 
     // alias=
     // unit=
     // comment=
     if (!variable->alias.empty())
         element->setAttributeCheck("alias", variable->alias);
     if (!variable->unit.empty())
         element->setAttributeCheck("unit", variable->unit);
     if (!variable->comment.empty())
         element->setAttributeCheck("comment", variable->comment);
 
     // datasize=
     element->setAttribute("datasize", variable->width);
 
     // typ=
     const char *dtype = 0;
     switch (variable->dtype) {
         case PdServ::Variable::boolean_T: dtype = "TCHAR";     break;
         case PdServ::Variable::uint8_T:   dtype = "TUCHAR";    break;
         case PdServ::Variable::int8_T:    dtype = "TCHAR";     break;
         case PdServ::Variable::uint16_T:  dtype = "TUSHORT";   break;
         case PdServ::Variable::int16_T:   dtype = "TSHORT";    break;
         case PdServ::Variable::uint32_T:  dtype = "TUINT";     break;
         case PdServ::Variable::int32_T:   dtype = "TINT";      break;
         case PdServ::Variable::uint64_T:  dtype = "TULINT";    break;
         case PdServ::Variable::int64_T:   dtype = "TLINT";     break;
         case PdServ::Variable::single_T:  dtype = "TFLT";      break;
         case PdServ::Variable::double_T:  dtype = "TDBL";      break;
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

         // Transmit either a vector or a matrix
         if (variable->ndims == 1) {
             cnum = variable->nelem;
             rnum = 1;
             orientation = "VECTOR";
         }
         else {
             const size_t *dim = variable->getDim();
             cnum = dim[variable->ndims - 1];
             rnum = variable->nelem / cnum;
             orientation = "MATRIX_ROW_MAJOR";
         }

         element->setAttribute("cnum", cnum);
         element->setAttribute("rnum", rnum);
         element->setAttribute("orientation", orientation);
     }
 
     // unit=
     if (!variable->unit.empty())
         element->setAttributeCheck("unit", variable->unit);
 
     // text=
     if (!variable->comment.empty())
         element->setAttributeCheck("text", variable->comment);
 
     // hide=
     // unhide=
}

/////////////////////////////////////////////////////////////////////////////
void Variable::base64Attribute(XmlElement* element, const char *name,
        size_t nblocks, const void *data) const
{
     static const char *base64Chr = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
         "abcdefghijklmnopqrstuvwxyz0123456789+/";
     size_t count = memSize * nblocks;
     size_t len = (count + 2 - ((count + 2) % 3)) / 3 * 4;
     char s[len + 1];
     char *p = s;
     size_t i = 0;
     size_t rem = count % 3;
     const unsigned char *buf = reinterpret_cast<const unsigned char*>(data);
 
     // First convert all characters in chunks of 3
     while (i != count - rem) {
         *p++ = base64Chr[  buf[i  ]         >> 2];
         *p++ = base64Chr[((buf[i  ] & 0x03) << 4) + (buf[i+1] >> 4)];
         *p++ = base64Chr[((buf[i+1] & 0x0f) << 2) + (buf[i+2] >> 6)];
         *p++ = base64Chr[ (buf[i+2] & 0x3f)     ];
 
         i += 3;
     }
 
     // Convert the remaining 1 or 2 characters
     switch (rem) {
         case 2:
             *p++ = base64Chr[  buf[i  ]         >> 2];
             *p++ = base64Chr[((buf[i  ] & 0x03) << 4) + (buf[i+1] >> 4)];
             *p++ = base64Chr[ (buf[i+1] & 0x0f) << 2];
             break;
         case 1:
             *p++ = base64Chr[  buf[i]         >> 2];
             *p++ = base64Chr[ (buf[i] & 0x03) << 4];
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
void Variable::hexDecAttribute(XmlElement* element, const char *name,
        size_t nblocks, const void *data) const
{
     std::string s;
     const unsigned char *buf =
         reinterpret_cast<const unsigned char*>(data);
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
 
     size_t charCount = memSize * nblocks * 2;
 
     s.reserve(charCount + 1);
 
     while (charCount--)
         s.append(hexValue[*buf++], 2);
 
     element->setAttribute(name, s);
}

/////////////////////////////////////////////////////////////////////////////
void Variable::csvAttribute( XmlElement *element, const char *name,
        size_t nblocks, const void *data, size_t precision) const
{
    std::ostringstream os;

    size_t idx = 0;
    for (size_t n = 0; n < nblocks; ++n) {
        for (size_t i = 0; i < nelem; ++i) {
            if (idx)
                os << ',';
            printFunc(os, data, idx++, precision);
        }
    }

    element->setAttribute(name, os.str());
}
