/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdserv library.
 *
 *  The pdserv library is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or (at
 *  your option) any later version.
 *
 *  The pdserv library is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with the pdserv library. If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#ifndef MSRINBUF_H
#define MSRINBUF_H

#include <stdint.h>
#include <cstddef>
#include <list>
#include <string>

namespace MsrProto {

class XmlParser {
    public:
        XmlParser(size_t max = bufIncrement * 1000);
        XmlParser(const char *begin, const char *end);
        ~XmlParser();

        char *bufptr();
        size_t free();

        void newData(size_t n);

        typedef std::pair<const char*, const char*> Attribute;
        typedef std::list<Attribute> AttributeList;

        class Element {
            public:
                Element(const char *command = 0,
                        const AttributeList* attribute = 0);

                operator bool() const;

                const char *getCommand() const;
                bool find(const char *name, const char **value = 0) const;
                bool isEqual(const char *name, const char *s) const;
                bool isTrue(const char *name) const;
                bool getString(const char *name, std::string &s) const;
                bool getUnsigned(const char *name, unsigned int &i) const;
                bool getUnsignedList(const char *name,
                        std::list<unsigned int> &i) const;

            private:
                const char * command;
                const AttributeList* attribute;
        };

        Element nextElement();

    private:
        const size_t bufLenMax;

        AttributeList attribute;

        static const size_t bufIncrement = 1024;
        size_t bufLen;
        char *buf;

        char *parsePos;        // Current parsing position
        char *inputEnd;        // End of input data
        char *element;         // pointer to '<' in the element
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
