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
#include <cstring>
#include <cstdio>
#include <assert.h>
#include <stdarg.h>

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

using namespace MsrProto;

int test_single(const char *s, ...)
{
    va_list ap;
    XmlParser inbuf;
    XmlParser::Element command;
    int i = 0;

    va_start(ap, s);

    printf("%s\n", s);

    for( i = 0; s[i]; ++i) {
        *inbuf.bufptr() = s[i];
        inbuf.newData(1);
        command = inbuf.nextElement();

        if (command) {
            int idx = va_arg(ap, int);
            printf("idx=%i i=%i\n", idx, i);
            assert(idx and idx == i);
        }
    }

    i = va_arg(ap, int);
    assert(!i);

    va_end(ap);
    return 1;
}

int main(int , const char *[])
{
    XmlParser inbuf;
    XmlParser::Element command;
    const char *s;
    char *buf;

    // Perfectly legal statement
    s = "<rp index=\"134\" value=13>";
    test_single(s, 24, 0);
    buf = inbuf.bufptr();
    assert( buf);                // Buffer may not be null
    assert( inbuf.free());       // and have some space
    strcpy( inbuf.bufptr(), s);
    inbuf.newData(14);
    assert(!inbuf.nextElement());
    inbuf.newData(11);
    assert((command = inbuf.nextElement()));
    assert(!strcmp(command.getCommand(), "rp"));  // which is "rp"
    assert(!inbuf.nextElement());              // and no next command
    assert( buf == inbuf.bufptr());      // Buffer pointer does not change

    // An illegal command - no space after '<'
    s = "< rp>";
    test_single(s, 0);
    strcpy( inbuf.bufptr(), s);
    inbuf.newData(strlen(s));
    assert(!inbuf.nextElement());
    assert(buf == inbuf.bufptr());      // Buffer pointer does not change

    // Two legal commands
    s = "<a_long_command_with_slash /><wp >";
    test_single(s, 28, 33, 0);
    strcpy( inbuf.bufptr(), s);
    inbuf.newData(strlen(s));
    assert((command = inbuf.nextElement()));
    assert(command.getCommand());
    assert(!strcmp(command.getCommand(), "a_long_command_with_slash"));
    assert((command = inbuf.nextElement()));
    assert(command.getCommand());
    assert(!strcmp(command.getCommand(), "wp"));
    assert(!inbuf.nextElement());

    s = " lkj <wrong/ >\n<still-incorrect /> <right> lkjs dfkl";
    test_single(s, 41, 0);
    strcpy( inbuf.bufptr(), s);
    inbuf.newData(strlen(s));
    assert((command = inbuf.nextElement()));
    assert(command.getCommand());
    assert(!strcmp(command.getCommand(), "right"));
    assert(!inbuf.nextElement());

    for (s = "<tag true with=no-quote-attr "
            "trueval=1 falseval=0 truestr=True falsestr=nottrue onstr='on' "
            " and=\"quoted /> > &quot; &apos;\" />";
            *s; s++) {
        *inbuf.bufptr() = *s;
        inbuf.newData(1); // == (*s == '>' and !s[1]);
        assert(!s[1] or !inbuf.nextElement());
    }
    assert((command = inbuf.nextElement()));
    assert(command.getCommand());
    assert(!strcmp(command.getCommand(), "tag"));
    assert(!command.isTrue("with"));
    assert(!command.isTrue("unknown"));
    assert( command.isTrue("true"));
    assert( command.isTrue("trueval"));
    assert(!command.isTrue("falseval"));
    assert( command.isTrue("truestr"));
    assert(!command.isTrue("falsestr"));
    assert( command.isTrue("onstr"));
    assert(command.find("and", &s));
    assert(!strcmp(s, "quoted /> > &quot; &apos;"));
    std::string str;
    assert(command.getString("and", str));
    assert(str == "quoted /> > \" '");
    assert(!inbuf.nextElement());

    s = "<tag with=no-quot/>attr and=\"quo\\\"ted /> > &quot; &apos;\" />";
    test_single(s, 18, 0);
    strcpy( inbuf.bufptr(), s);
    inbuf.newData(strlen(s));
    assert((command = inbuf.nextElement()));
    assert(command.getCommand());
    assert(!strcmp(command.getCommand(), "tag"));
    assert(command.isTrue("tag"));        // "tag" is also an attribute
    assert(command.find("with", &s));
    assert(!strcmp(s, "no-quot"));
    assert(!inbuf.nextElement());

    s = "<with=no-quot-attr and=\"quoted /> > &quot; &apos;\" />";
    test_single(s, 52, 0);
    strcpy( inbuf.bufptr(), s);
    inbuf.newData(strlen(s));
    assert((command = inbuf.nextElement()));
    assert(command.getCommand());        // There is no command
    assert(command.find("with", &s));
    assert(!strcmp(s, "no-quot-attr"));
    assert(!inbuf.nextElement());
    return 0;
}
