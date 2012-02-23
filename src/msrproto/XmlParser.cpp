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

#include "XmlParser.h"
#include "Session.h"
#include "../Debug.h"

#include <algorithm>

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
XmlParser::XmlParser(size_t bufMax): bufLenMax(bufMax)
{
    parseState = FindStart;
    buf = new char[bufIncrement];
    bufLen = bufIncrement;

    parsePos = 0;
    inputEnd = 0;
}

/////////////////////////////////////////////////////////////////////////////
XmlParser::XmlParser(const char *begin, const char *end):
    bufLenMax(end - begin)
{
    parseState = FindStart;
    buf = new char[bufLenMax];
    bufLen = bufLenMax;

    parsePos = 0;
    inputEnd = 0;

    std::copy(begin, end, buf);
    newData(bufLen);
}

/////////////////////////////////////////////////////////////////////////////
XmlParser::~XmlParser()
{
    delete[] buf;
}

/////////////////////////////////////////////////////////////////////////////
size_t XmlParser::free()
{
    return bufLen - inputEnd;
}

/////////////////////////////////////////////////////////////////////////////
char *XmlParser::bufptr()
{
    return buf + inputEnd;
}

/////////////////////////////////////////////////////////////////////////////
const char *XmlParser::getCommand() const
{
    return buf + commandPos;
}

/////////////////////////////////////////////////////////////////////////////
bool XmlParser::find(const char *name, const char *&value) const
{
    AttributeMap::const_iterator it = attribute.find(*name);
    if (it == attribute.end())
        return false;

    const AttributePos& pos = it->second;

    for (AttributePos::const_iterator it = pos.begin();
            it != pos.end(); ++it) {
        if (!strcasecmp(name, buf + it->first)) {
            value = it->second ? buf + it->second : 0;
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

    if (!(find(name, value) and value))
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
void XmlParser::newData(size_t n)
{
    inputEnd += n;
}

/////////////////////////////////////////////////////////////////////////////
bool XmlParser::next()
{
    bool finished = false;

    while (parsePos < inputEnd) {

        char& c = buf[parsePos++];

        switch (parseState) {
            case FindStart:
                if (c == '<') {
                    // Found beginning
                    attribute.clear();
                    commandPos = parsePos;
                    parseState = ExpectToken;
                }
                else {
                    // Move forward in the buffer until '<' is found
                    parsePos =
                        std::find(buf + parsePos, buf + inputEnd, '<') - buf;
                }
                break;

            case ExpectToken:
                // Moves forward until an alpha character is found
                if (isalpha(c)) {
                    attrNamePos = &c - buf;
                    parseState = FindTokenEnd;
                    attribute[buf[attrNamePos]][attrNamePos] = 0;
                }
                else
                    parseState = FindStart;
                break;

            case SkipSpace:
                // Check for end of XML Element
                if (isalpha(c)) {
                    parseState = ExpectToken;
                    --parsePos;
                    break;
                }
                else if (c == '/')
                    parseState = ExpectGT;
                else if (c == '>')
                    finished = true;
                else if (!isspace(c))
                    parseState = FindStart;

                c = '\0';
                break;

            case FindTokenEnd:
                // Moves forward until non-alpha is found
                if (isalpha(c) or c == '_')
                    break;

                if (c == '=') {
                    parseState = GetAttribute;
                    c = '\0';
                }
                else {
                    parseState = SkipSpace;
                    --parsePos;
                }

                break;

            case GetAttribute:
                {
                    size_t attrValuePos;
                    if (c == '\'' or c == '"') {
                        quote = c;
                        attrValuePos = parsePos;
                        parseState = GetQuotedAttribute;
                    }
                    else {
                        attrValuePos = &c - buf;
                        parseState = GetUnquotedAttribute;
                    }

                    attribute[buf[attrNamePos]][attrNamePos] = attrValuePos;
                }

                break;
                
            case GetQuotedAttribute:
                // Skip Escape char
                if (c == '\\') {
                    escapeState = GetQuotedAttribute;
                    parseState = SkipEscapeChar;
                    break;
                }

                if (c == quote) {
                    c = '\0';

                    parseState = SkipSpace;
                }
                break;

            case GetUnquotedAttribute:
                if (c == '>') {
                    c = '\0';
                    finished = true;
                }
                else if (c == '/')
                    parseState = MaybeGT;
                else if (isspace(c)) {
                    c = '\0';
                    parseState = SkipSpace;
                }

                break;

            case ExpectGT:
                parseState = FindStart;
                finished = c == '>';

                break;

            case MaybeGT:
                if (c == '>') {
                    *(&c - 1) = '\0';
                    finished = true;
                }
                else
                    parseState = GetUnquotedAttribute;
                break;

            case SkipEscapeChar:
                parseState = escapeState;
                break;
        }

        if (finished) {
            parseState = FindStart;
            return true;
        }
    }

    if (parseState == FindStart)
        parsePos = inputEnd = 0;
    else if (inputEnd == bufLen) {
        if (commandPos > 1) {
            size_t s = commandPos - 1;

            std::copy(buf + s, buf + inputEnd, buf);

            parsePos -= s;
            inputEnd -= s;
            commandPos = 1;
            attrNamePos -= s;
            for (AttributeMap::iterator it = attribute.begin();
                    it != attribute.end(); ++it) {
                AttributePos::iterator i, it2 = it->second.begin();
                while (it2 != it->second.end()) {
                    (it->second)[it2->first - s] = it2->second - s;
                    i = it2++;
                    it->second.erase(i);
                }
            }
        }
        else {
            bufLen += bufIncrement;

            if (bufLen > bufLenMax) {
                bufLen = bufIncrement;
                parseState = FindStart;
                parsePos = inputEnd = 0;
            }

            char *data = new char[bufLen];
            std::copy(buf, buf + inputEnd, data);

            delete[] buf;
            buf = data;
        }
    }

    return false;
}
