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

#include "XmlParser.h"
#include "Session.h"

#include <algorithm>

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
XmlParser::XmlParser(size_t bufMax): bufLenMax(bufMax)
{
    parseState = FindStart;
    buf = new char[bufIncrement];
    bufLen = bufIncrement;

    begin = 0;
    parsePos = 0;
    inputEnd = 0;
}

/////////////////////////////////////////////////////////////////////////////
XmlParser::XmlParser(const std::string &s, size_t offset):
    bufLenMax(s.size() - offset)
{
    parseState = FindStart;
    buf = new char[bufLenMax];
    bufLen = bufLenMax;

    begin = 0;
    parsePos = 0;
    inputEnd = 0;

    std::copy(s.begin() + offset, s.end(), buf);
    newData(bufLen);
}

/////////////////////////////////////////////////////////////////////////////
XmlParser::~XmlParser()
{
    delete[] buf;
}

/////////////////////////////////////////////////////////////////////////////
void XmlParser::checkFreeSpace()
{
    if (inputEnd == bufLen) {
        bufLen += bufIncrement;

        if (bufLen > bufLenMax) {
            bufLen = bufIncrement;
            parseState = FindStart;
            begin = parsePos = inputEnd = 0;
        }

        char *data = new char[bufLen];
        std::copy(buf, buf + inputEnd, data);
        
        delete[] buf;
        buf = data;
    }
}

/////////////////////////////////////////////////////////////////////////////
size_t XmlParser::free()
{
    checkFreeSpace();
    return bufLen - inputEnd;
}

/////////////////////////////////////////////////////////////////////////////
char *XmlParser::bufptr()
{
    checkFreeSpace();
    return buf + inputEnd;
}

/////////////////////////////////////////////////////////////////////////////
const char *XmlParser::getCommand() const
{
    return commandPos ? buf + commandPos : 0;
}

/////////////////////////////////////////////////////////////////////////////
bool XmlParser::find(const char *name, const char * &value) const
{
    std::pair<AttributeMap::const_iterator, AttributeMap::const_iterator>
        ret(attribute.equal_range(*name));

    for (AttributeMap::const_iterator it(ret.first); it != ret.second; it++) {
        if (!strcasecmp(name, buf + it->second.namePos)) {
            value = it->second.valuePos ? buf + it->second.valuePos : 0;
            return true;
        }
    }

    return false;
}

/////////////////////////////////////////////////////////////////////////////
bool XmlParser::isEqual(const char *name, const char *s) const
{
    const char *value;

    if (find(name, value) and value)
        return !strcasecmp(value, s);

    return false;
}

/////////////////////////////////////////////////////////////////////////////
bool XmlParser::isTrue(const char *name) const
{
    const char *value;

    if (!(find(name, value)))
        return false;

    // Binary attribute, e.g <xsad sync>
    if (!value)
        return true;

    size_t len = strlen(value);

    // Binary attribute, e.g <xsad sync=1>
    if (len == 1)
        return *value == '1';

    // Binary attribute, e.g <xsad sync="true">
    if (len == 4)
        return !strncasecmp(value, "true", 4);

    // Binary attribute, e.g <xsad sync='on'/>
    if (len == 2)
        return !strncasecmp(value, "on", 2);

    return false;
}

/////////////////////////////////////////////////////////////////////////////
bool XmlParser::getString(const char *name, std::string &s) const
{
    const char *value;

    s.clear();

    if (!(find(name, value)) or !value)
        return false;

    const char *pptr, *eptr = value + strlen(value);
    while ((pptr = std::find(value, eptr, '&')) != eptr) {
        s.append(value, pptr - value);
        size_t len = eptr - pptr;
        if (len >= 4 and !strncmp(pptr, "&gt;", 4)) {
            s.append(1, '>');
            value = pptr + 4;
        }
        else if (len >= 4 and !strncmp(pptr, "&lt;", 4)) {
            s.append(1, '<');
            value = pptr + 4;
        }
        else if (len >= 5 and !strncmp(pptr, "&amp;", 5)) {
            s.append(1, '&');
            value = pptr + 5;
        }
        else if (len >= 6 and !strncmp(pptr, "&quot;", 6)) {
            s.append(1, '"');
            value = pptr + 6;
        }
        else if (len >= 6 and !strncmp(pptr, "&apos;", 6)) {
            s.append(1, '\'');
            value = pptr + 6;
        }
        else {
            s.append(1, '&');
            value = pptr + 1;
        }
    }

    s.append(value, eptr - value);
    return true;
}

/////////////////////////////////////////////////////////////////////////////
bool XmlParser::getUnsigned(const char *name, unsigned int &i) const
{
    const char *value;

    if (!(find(name, value)) or !value)
        return false;

    i = strtoul(value, 0, 0);
    return true;
}

/////////////////////////////////////////////////////////////////////////////
bool XmlParser::getUnsignedList(const char *name,
        std::list<unsigned int> &intList) const
{
    const char *value;

    if (!(find(name, value)) or !value)
        return false;

    std::istringstream is(value);
    is.imbue(std::locale::classic());

    while (is) {
        unsigned int i;
        char comma;

        is >> i;
        if (is)
            intList.push_back(i);
        is >> comma;
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////
bool XmlParser::next()
{
    begin = parsePos;
    parseState = FindStart;
    if (begin >= inputEnd) {
        begin = parsePos = inputEnd = 0;
        return false;
    }

    return newData(0);
}

/////////////////////////////////////////////////////////////////////////////
bool XmlParser::newData(size_t n)
{
    inputEnd += n;

    while (true) {
        switch (parseState) {
            case FindStart:
                //cout << __LINE__ << "FindStart" << endl;

                attribute.clear();
                
                // Move forward in the buffer until '<' is found
                parsePos = std::find(buf + parsePos, buf + inputEnd, '<') - buf;
                if (begin == inputEnd) {
                    begin = parsePos = inputEnd = 0;
                    return false;
                }

                begin = parsePos;
                commandPos = 0;
                parseState = GetToken;

                parsePos = begin + 1;
                attrNamePos = parsePos;
                // no break here

            case GetToken:
                do {
                    if (parsePos == inputEnd)
                        return false;
                    if (!isalpha(buf[parsePos]) and buf[parsePos] != '_')
                        break;
                    parsePos++;
                } while (1);

                // Decide what to do with the next character
                if (buf[parsePos] == '=') {
                    parseState = GetQuote;
                }
                else {
                    if (!commandPos and attrNamePos != parsePos)
                        commandPos = attrNamePos;

                    if (buf[parsePos] == '>') {
                        buf[parsePos++] = 0;

                        std::pair<char, AttributePos> ap( buf[attrNamePos],
                                AttributePos(attrNamePos, 0));
                        attribute.insert(ap);
                        begin = parsePos;
                        parseState = FindStart;
                        return true;
                    }
                    else if (buf[parsePos] == '/') {
                        if (inputEnd - parsePos < 2)
                            return false;

                        if (buf[parsePos+1] != '>') {
                            // Error: start over again
                            parsePos++; // Skip the first '/' only
                            parseState = FindStart;
                            break;
                        }

                        buf[parsePos] = 0;
                        parsePos += 2;

                        std::pair<char, AttributePos> ap( buf[attrNamePos],
                                AttributePos(attrNamePos, 0));
                        attribute.insert(ap);
                        parseState = FindStart;
                        return true;
                    }
                    else if (isspace(buf[parsePos])) {
                        if (parsePos == attrNamePos) {
                            // No new attribute - discard
                            parseState = FindStart;
                            break;
                        }

                        buf[parsePos] = 0;
                        std::pair<char, AttributePos> ap( buf[attrNamePos],
                                AttributePos(attrNamePos, 0));
                        attribute.insert(ap);
                        parseState = SkipSpace;
                    }
                    else {
                        parseState = FindStart;
                    }
                }

                buf[parsePos++] = 0;
                break;

            case SkipSpace:
                //cout << __LINE__ << "GetCommand" << endl;
                do {
                    if (parsePos == inputEnd)
                        return false;
                    if (!isspace(buf[parsePos]))
                        break;
                    parsePos++;
                } while(1);

                attrNamePos = parsePos;
                parseState = GetToken;
                break;
                // no break

            case GetQuote:
                if (parsePos == inputEnd)
                    return false;

                if (buf[parsePos] == '\'' or buf[parsePos] == '"') {
                    quote = buf[parsePos];
                    attrValuePos = ++parsePos;
                }
                else {
                    quote = 0;
                    attrValuePos = parsePos;
                }

                parseState = GetAttribute;
                // no break;
                
            case GetAttribute:
                do {
                    if (parsePos == inputEnd)
                        return false;

                    if (quote) {
                        if (buf[parsePos] == quote)
                            break;
                    }
                    else if (isspace(buf[parsePos]))
                        break;
                    else if (buf[parsePos] == '>') {
                        parseState = FindStart;
                        break;
                    }
                    else if (buf[parsePos] == '/') {
                        parseState = GetToken;
                        break;
                    }

                    parsePos++;
                } while (1);

                std::pair<char, AttributePos> ap( buf[attrNamePos],
                        AttributePos(attrNamePos, attrValuePos));
                attribute.insert(ap);
                if (parseState != GetToken) {
                    buf[parsePos++] = 0;
                    if (parseState == FindStart)
                        return true;
                    parseState = SkipSpace;
                }
                break;
        }
    }
}
