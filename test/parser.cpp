#include "XmlParser.h"
#include <cstring>
#include <cstdio>
#include <assert.h>

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

using namespace MsrProto;

int main(int argc, const char *argv[])
{
    XmlParser inbuf;
    const char *s;
    char *buf;

    // Perfectly legal statement
    s = "<rp index=\"134\" value=13>";
    buf = inbuf.bufptr();
    assert( buf);                // Buffer may not be null
    assert( inbuf.free());       // and have some space
    strcpy( inbuf.bufptr(), s);
    inbuf.newData(14);
    assert(!inbuf.next());
    inbuf.newData(11);
    assert( inbuf.next());
    assert( inbuf.getCommand());         // There is a command
    assert(!strcmp(inbuf.getCommand(), "rp"));  // which is "rp"
    assert(!inbuf.next());              // and no next command
    assert( buf == inbuf.bufptr());      // Buffer pointer does not change

    // An illegal command - no space after '<'
    s = "< rp>";
    strcpy( inbuf.bufptr(), s);
    inbuf.newData(strlen(s));
    assert(!inbuf.next());
    assert(buf == inbuf.bufptr());      // Buffer pointer does not change

    // Two legal commands
    s = "<a_long_command_with_slash /><wp >";
    strcpy( inbuf.bufptr(), s);
    inbuf.newData(strlen(s));
    assert(inbuf.next());
    assert(inbuf.getCommand());
    assert(!strcmp(inbuf.getCommand(), "a_long_command_with_slash"));
    assert(inbuf.next());
    assert(inbuf.getCommand());
    assert(!strcmp(inbuf.getCommand(), "wp"));
    assert(!inbuf.next());

    s = " lkj <wrong/ >\n<still-incorrect /> <right> lkjs dfkl";
    strcpy( inbuf.bufptr(), s);
    inbuf.newData(strlen(s));
    assert(inbuf.next());
    assert(inbuf.getCommand());
    assert(!strcmp(inbuf.getCommand(), "right"));
    assert(!inbuf.next());

    for (s = "<tag true with=no-quote-attr "
            "trueval=1 falseval=0 truestr=True falsestr=nottrue onstr='on' "
            " and=\"quoted /> > &quot; &apos;\" />";
            *s; s++) {
        *inbuf.bufptr() = *s;
        inbuf.newData(1); // == (*s == '>' and !s[1]);
        assert(!s[1] or !inbuf.next());
    }
    assert(inbuf.next());
    assert(inbuf.getCommand());
    assert(!strcmp(inbuf.getCommand(), "tag"));
    assert(!inbuf.isTrue("with"));
    assert(!inbuf.isTrue("unknown"));
    assert( inbuf.isTrue("true"));
    assert( inbuf.isTrue("trueval"));
    assert(!inbuf.isTrue("falseval"));
    assert( inbuf.isTrue("truestr"));
    assert(!inbuf.isTrue("falsestr"));
    assert( inbuf.isTrue("onstr"));
    assert(inbuf.find("and", s));
    assert(!strcmp(s, "quoted /> > &quot; &apos;"));
    std::string str;
    assert(inbuf.getString("and", str));
    assert(str == "quoted /> > \" '");
    assert(!inbuf.next());

    s = "<tag with=no-quot/>attr and=\"quoted /> > &quot; &apos;\" />";
    strcpy( inbuf.bufptr(), s);
    inbuf.newData(strlen(s));
    assert(inbuf.next());
    assert(inbuf.getCommand());
    assert(!strcmp(inbuf.getCommand(), "tag"));
    assert(inbuf.isTrue("tag"));        // "tag" is also an attribute
    assert(inbuf.find("with", s));
    assert(!strcmp(s, "no-quot"));
    assert(!inbuf.next());

    s = "<with=no-quot-attr and=\"quoted /> > &quot; &apos;\" />";
    strcpy( inbuf.bufptr(), s);
    inbuf.newData(strlen(s));
    assert(inbuf.next());
    assert(inbuf.getCommand());        // There is no command
    assert(inbuf.find("with", s));
    assert(!strcmp(s, "no-quot-attr"));
    assert(!inbuf.next());
    return 0;
}
