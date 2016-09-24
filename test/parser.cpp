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

#include <sstream>
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

using namespace MsrProto;

int test_single(const char *s, ...)
{
    va_list ap;
    XmlParser inbuf;
    std::stringbuf input;
    int i = 0;

    va_start(ap, s);

    printf("%s\n", s);

    for( i = 0; s[i]; ++i) {
        input.sputc(s[i]);
        inbuf.read(&input);

        if (inbuf) {
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
    std::stringstream buffer;
    XmlParser inbuf;
    const char *s;

    // Perfectly legal statement
    s = "<rp index=\"134\" value=13 argument>";
    test_single(s, 33, 0);
    buffer << s;
    inbuf.read(buffer.rdbuf());
    assert(inbuf);
    assert(!inbuf);

    buffer << "<>";
    inbuf.read(buffer.rdbuf());
    assert(!inbuf);

    buffer << "</>";
    inbuf.read(buffer.rdbuf());
    assert(!inbuf);

    buffer << "<valid/>";
    inbuf.read(buffer.rdbuf());
    assert(inbuf);
    assert(!strcmp(inbuf.tag(), "valid"));
    assert(!inbuf);

    buffer << "<valid  />";
    inbuf.read(buffer.rdbuf());
    assert(inbuf);
    assert(!strcmp(inbuf.tag(), "valid"));
    assert(!inbuf);

    buffer << "lks flk s <valid/>";
    inbuf.read(buffer.rdbuf());
    assert(inbuf);
    assert(!strcmp(inbuf.tag(), "valid"));
    assert(!inbuf);

    buffer << "< invalid>";
    inbuf.read(buffer.rdbuf());
    assert(!inbuf);

    buffer << "<1nvalid>";
    inbuf.read(buffer.rdbuf());
    assert(!inbuf);

    buffer << "<valid-boolean-argument argument>";
    inbuf.read(buffer.rdbuf());
    assert(inbuf);
    assert(!strcmp(inbuf.tag(), "valid-boolean-argument"));
    assert(inbuf.isTrue("argument"));
    assert(!inbuf);

    buffer << "<valid-argument argum<ent>";
    inbuf.read(buffer.rdbuf());
    assert(inbuf);
    assert(!strcmp(inbuf.tag(), "valid-argument"));
    assert(inbuf.isTrue("argum<ent"));
    assert(!inbuf);

    buffer << "<invalid-argument 1argu>";
    inbuf.read(buffer.rdbuf());
    assert(!inbuf);
    assert(!inbuf);

    buffer << "<valid-argument argument=value>";
    inbuf.read(buffer.rdbuf());
    assert(inbuf);
    assert(!strcmp(inbuf.tag(), "valid-argument"));
    assert(inbuf.isEqual("argument", "value"));
    assert(!inbuf);

    buffer << "<valid-argument argument=\"val'ue\">";
    inbuf.read(buffer.rdbuf());
    assert(inbuf);
    assert(!strcmp(inbuf.tag(), "valid-argument"));
    assert(inbuf.isEqual("argument", "val'ue"));
    assert(!inbuf);

    buffer << "<valid-argument argument='va\"lu>e with space'>";
    inbuf.read(buffer.rdbuf());
    assert(inbuf);
    assert(!strcmp(inbuf.tag(), "valid-argument"));
    assert(inbuf.isEqual("argument", "va\"lu>e with space"));
    assert(!inbuf);

    buffer << "<starttls invalidtls><notls>";
    inbuf.read(buffer.rdbuf());
    assert(!inbuf);
    assert(inbuf);
    assert(!strcmp(inbuf.tag(), "notls"));
    assert(!inbuf);

    buffer << "<starttls validargument>\n";
    inbuf.read(buffer.rdbuf());
    assert(inbuf);
    assert(!strcmp(inbuf.tag(), "starttls"));
    assert(inbuf.isTrue("validargument"));
    assert(!inbuf);

    buffer << "<starttls>\r\n";
    inbuf.read(buffer.rdbuf());
    assert(inbuf);
    assert(!strcmp(inbuf.tag(), "starttls"));
    assert(!inbuf);

    buffer << " lkj <wrong/ >\n<correct /> <right> lkjs dfkl";
    inbuf.read(buffer.rdbuf());
    assert(inbuf);
    assert(!strcmp(inbuf.tag(), "correct"));
    assert(inbuf);
    assert(!strcmp(inbuf.tag(), "right"));
    assert(!inbuf);

    for (s = "<tag true with=no-quote-attr "
            "trueval=1 falseval=0 truestr=True falsestr=nottrue onstr='on' "
            " and=\"quoted /> > &quot; &apos;\" />";
            *s; s++) {
        buffer << *s;
        inbuf.read(buffer.rdbuf());
        assert(!s[1] xor !inbuf);
    }
    assert(!strcmp(inbuf.tag(), "tag"));
    assert(!inbuf.isTrue("with"));
    assert(!inbuf.isTrue("unknown"));
    assert( inbuf.isTrue("true"));
    assert( inbuf.isTrue("trueval"));
    assert(!inbuf.isTrue("falseval"));
    assert( inbuf.isTrue("truestr"));
    assert(!inbuf.isTrue("falsestr"));
    assert( inbuf.isTrue("onstr"));
    assert(inbuf.find("and", &s));
    assert(!strcmp(s, "quoted /> > &quot; &apos;"));
    std::string str;
    assert(inbuf.getString("and", str));
    assert(str == "quoted /> > \" '");
    assert(!inbuf);

    buffer
        << "<tag with=no-quot/>attr and=\"quo\\\"ted /> > &quot; &apos;\" />";
    inbuf.read(buffer.rdbuf());
    assert(inbuf);
    assert(!strcmp(inbuf.tag(), "tag"));
    assert(inbuf.isTrue("tag"));        // "tag" is also an attribute
    assert(inbuf.find("with", &s));
    assert(!strcmp(s, "no-quot"));
    assert(!inbuf);

    buffer << "<with=no-quot-attr and=\"quoted /> > &quot; &apos;\" />";
    inbuf.read(buffer.rdbuf());
    assert(inbuf);
    assert(!strcmp(inbuf.tag(), "with"));
    assert(inbuf.find("with", &s));
    assert(!strcmp(s, "no-quot-attr"));
    assert(!inbuf);

    return 0;
}
