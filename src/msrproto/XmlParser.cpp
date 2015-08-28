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
#include <cstring>

#undef log_debug
#define log_debug(...)

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
XmlParser::XmlParser(size_t bufMax): bufLenMax(bufMax)
{
    parseState = FindStart;
    buf = new char[bufIncrement];
    bufLen = bufIncrement;

    parsePos = buf;
    inputEnd = buf;
}

/////////////////////////////////////////////////////////////////////////////
XmlParser::XmlParser(const char *begin, const char *end):
    bufLenMax(end - begin)
{
    parseState = FindStart;
    buf = new char[bufLenMax];
    bufLen = bufLenMax;

    parsePos = buf;
    inputEnd = buf;

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
    return (buf + bufLen) - inputEnd;
}

/////////////////////////////////////////////////////////////////////////////
char *XmlParser::bufptr()
{
    return inputEnd;
}

/////////////////////////////////////////////////////////////////////////////
void XmlParser::newData(size_t n)
{
    inputEnd += n;
}

/////////////////////////////////////////////////////////////////////////////
XmlParser::Element XmlParser::nextElement()
{
    bool finished = false;

    for (; parsePos < inputEnd and !finished; ++parsePos) {
//        log_debug("state %i, char %c", parseState, *parsePos);

        switch (parseState) {
            case FindStart:
                if (*parsePos == '<') {
                    // Found beginning
                    attribute.clear();
                    element = parsePos;
                    parseState = ExpectToken;
                }
                else {
                    // Move forward in the buffer until '<' is found
                    parsePos = std::find(parsePos+1, inputEnd, '<') - 1;
//                    log_debug("Found < at %zu", parsePos - buf);
                }
                break;

            case ExpectToken:
                // Expect an alpha character here
                if (isalpha(*parsePos)) {
                    parseState = FindTokenEnd;
                    attribute.push_back(
                            std::make_pair(parsePos,(const char*)0));
                }
                else
                    parseState = FindStart;
                break;

            case SkipSpace:
                // Check for end of XML Element
                if (isalpha(*parsePos)) {
                    // Found next token
                    parseState = ExpectToken;

                    // Decrement parsePos because the pointer gets incremented
                    // automatically at the loop end
                    --parsePos;
                    break;
                }

                if (*parsePos == '/')
                    parseState = ExpectGT;
                else if (*parsePos == '>')
                    finished = true;
                else if (!isspace(*parsePos))
                    parseState = FindStart;

                *parsePos = '\0';       // Clobber spaces
                break;

            case FindTokenEnd:
                // Moves forward until non-alpha is found
                if (isalpha(*parsePos) or *parsePos == '_')
                    break;

                if (*parsePos == '=') {
                    parseState = GetAttribute;
                    *parsePos = '\0';
                }
                else {
                    parseState = SkipSpace;
                    --parsePos;
                }


                break;

            case GetAttribute:
                {
                    const char *attrValuePos = parsePos;
                    if (*parsePos == '\'' or *parsePos == '"') {
                        quote = *parsePos;
                        ++attrValuePos;
                        parseState = GetQuotedAttribute;
                    }
                    else {
                        parseState = GetUnquotedAttribute;
                    }

                    attribute.rbegin()->second = attrValuePos;
                }

                break;

            case GetQuotedAttribute:
                {
                    char *quotePos = std::find(parsePos, inputEnd, quote);
                    char *escape   = std::find(parsePos, quotePos, '\\');

                    parsePos = quotePos;

//                    log_debug("quotePos=%zu escape=%zu",
//                            quotePos - buf, escape - quotePos);
                    if (escape < quotePos) {
                        parsePos = escape;
                        escapeState = GetQuotedAttribute;
                        parseState = SkipEscapeChar;
                    }
                    else if (quotePos < inputEnd) {
                        *parsePos = '\0';

                        parseState = SkipSpace;
                    }
                    else
                        --parsePos;
                }
                break;

            case GetUnquotedAttribute:
                if (*parsePos == '>') {
                    *parsePos = '\0';
                    finished = true;
                }
                else if (*parsePos == '/')
                    parseState = MaybeGT;
                else if (isspace(*parsePos)) {
                    *parsePos = '\0';
                    parseState = SkipSpace;
                }

                break;

            case ExpectGT:
                parseState = FindStart;
                finished = *parsePos == '>';

                break;

            case MaybeGT:
                if (*parsePos == '>') {
                    *(parsePos - 1) = '\0';
                    finished = true;
                }
                else
                    parseState = GetUnquotedAttribute;
                break;

            case SkipEscapeChar:
                parseState = escapeState;
                break;
        }
    }

    if (finished) {
        log_debug("found something");
        parseState = FindStart;
        return Element(element+1, &attribute);
    }

    if (parseState == FindStart) {
        parsePos = inputEnd = buf;
        log_debug("resetting pointers");
    }
    else if (inputEnd == buf + bufLen) {
        if (element > buf) {
            size_t n = element - buf;

            std::copy(buf + n, inputEnd, buf);

            parsePos -= n;
            inputEnd -= n;
            element = buf;
            for (AttributeList::iterator it = attribute.begin();
                    it != attribute.end(); ++it) {
                it->first -= n;
                it->second -= n;
            }
            log_debug("move data to buf start");
        }
        else {
            // Note: element points to beginning of buf here

            bufLen += bufIncrement;

            if (bufLen > bufLenMax) {
                // Overrun max buffer
                bufLen = bufIncrement;
                parseState = FindStart;

                element    = buf;
                parsePos   = buf;
                inputEnd   = buf;
            }

            buf = new char[bufLen];
            std::copy(element, inputEnd, buf);

            parsePos = buf + (parsePos - element);
            inputEnd = buf + (inputEnd - element);
            for (AttributeList::iterator it = attribute.begin();
                    it != attribute.end(); ++it) {
                it->first  = buf + (it->first  - element);
                it->second = buf + (it->second - element);
            }

            delete[] element;
            element = buf;
            log_debug("increased buf to %zu", bufLen);
        }
    }

    log_debug("empty");
    return Element(0,0);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
XmlParser::Element::Element(
        const char *command, const XmlParser::AttributeList* attr):
    command(command), attribute(attr)
{
}

/////////////////////////////////////////////////////////////////////////////
XmlParser::Element::operator bool() const
{
    return command != 0;
}

/////////////////////////////////////////////////////////////////////////////
const char *XmlParser::Element::getCommand() const
{
    return command;
}

/////////////////////////////////////////////////////////////////////////////
bool XmlParser::Element::find(const char *name, const char **value) const
{
    for (AttributeList::const_iterator it = attribute->begin();
            it != attribute->end(); ++it) {
        if (!strcasecmp(name, it->first)) {
            if (value)
                *value = it->second;
            return true;
        }
    }

    return false;
}

/////////////////////////////////////////////////////////////////////////////
bool XmlParser::Element::isEqual(const char *name, const char *s) const
{
    const char *value;

    if (find(name, &value) and value)
        return !strcasecmp(value, s);

    return false;
}

/////////////////////////////////////////////////////////////////////////////
bool XmlParser::Element::isTrue(const char *name) const
{
    const char *value;

    if (!(find(name, &value)))
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
bool XmlParser::Element::getString(const char *name, std::string &s) const
{
    const char *value;

    s.clear();

    if (!(find(name, &value) and value))
        return false;

    const char *pptr, *eptr = value + strlen(value);
    while ((pptr = std::find(value, eptr, '&')) != eptr) {
        // FIXME: maybe also check for escape char, e.g. \" ??
        // this is difficult, because the quote character is not available
        // here any more :(

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
bool XmlParser::Element::getUnsigned(const char *name, unsigned int &i) const
{
    const char *value;

    if (!(find(name, &value)) or !value)
        return false;

    i = strtoul(value, 0, 0);
    return true;
}

/////////////////////////////////////////////////////////////////////////////
bool XmlParser::Element::getUnsignedList(const char *name,
        std::list<unsigned int> &intList) const
{
    const char *value;

    if (!(find(name, &value)) or !value)
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
