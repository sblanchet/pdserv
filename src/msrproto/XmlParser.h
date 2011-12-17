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

#ifndef MSRINBUF_H
#define MSRINBUF_H

#include <stdint.h>
#include <cstddef>
#include <map>
#include <list>
#include <string>

namespace MsrProto {

class Session;

class XmlParser {
    public:
        XmlParser(size_t max = bufIncrement * 1000);
        XmlParser(const std::string &s, size_t offset = 0);
        ~XmlParser();

        char *bufptr();
        size_t free();

        void newData(size_t n);
        bool next();

        const char *getCommand() const;
        bool find(const char *name, const char *&value) const;
        bool isEqual(const char *name, const char *s) const;
        bool isTrue(const char *name) const;
        bool getString(const char *name, std::string &s) const;
        bool getUnsigned(const char *name, unsigned int &i) const;
        bool getUnsignedList(const char *name,
                std::list<unsigned int> &i) const;

    private:
        const size_t bufLenMax;

        typedef std::map<size_t, size_t> AttributePos;
        typedef std::map<char, AttributePos> AttributeMap;
        AttributeMap attribute;

        static const size_t bufIncrement = 1024;
        size_t bufLen;
        char *buf;

        size_t parsePos;        // Current parsing position
        size_t inputEnd;        // End of input data
        size_t commandPos;
        size_t attrNamePos;
        char quote;

        enum ParseState {
            FindStart, ExpectToken, SkipSpace, FindTokenEnd, GetAttribute,
            GetQuotedAttribute, GetUnquotedAttribute,  ExpectGT, MaybeGT,
            SkipEscapeChar
        };
        ParseState parseState, escapeState;
};

}
#endif //MSRINBUF_H
