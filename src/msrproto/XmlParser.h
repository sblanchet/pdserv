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
        ~XmlParser();

        char *bufptr() const;
        size_t free() const;

        bool newData(size_t n);
        bool next();

        const char *getCommand() const;
        bool find(const char *name, const char * &value) const;
        bool isEqual(const char *name, const char *s) const;
        bool isTrue(const char *name) const;
        bool getString(const char *name, std::string &s) const;
        bool getUnsigned(const char *name, unsigned int &i) const;
        bool getUnsignedList(const char *name,
                std::list<unsigned int> &i) const;

    private:
        const size_t bufLenMax;

        struct AttributePos {
            AttributePos(size_t n, size_t v): namePos(n), valuePos(v) {}
            const size_t namePos;
            const size_t valuePos;
        };
        typedef std::multimap<char, AttributePos> AttributeMap;
        AttributeMap attribute;

        void tokenize();

        static const size_t bufIncrement = 1024;
        size_t bufLen;
        char *buf;

        size_t begin;           // Start of an element
        size_t parsePos;        // Current parsing position
        size_t inputEnd;        // End of input data

        enum ParseState {
            FindStart, GetToken, SkipSpace, GetAttribute, GetQuote
        };
        ParseState parseState;

        size_t commandPos;
        size_t attrNamePos, attrValuePos;
        char quote;
};

}
#endif //MSRINBUF_H
