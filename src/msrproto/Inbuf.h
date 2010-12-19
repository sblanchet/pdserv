/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef MSRINBUF_H
#define MSRINBUF_H

#include "Attribute.h"

#include <stdint.h>
#include <cstddef>

namespace MsrProto {

class Session;

class Inbuf {
    public:
        Inbuf(Session *, size_t max = bufIncrement * 1000);
        ~Inbuf();

        char *bufptr() const;
        size_t free() const;

        void parse(size_t n);

    private:
        Session * const session;
        const size_t bufLenMax;

        void tokenize();

        static const size_t bufIncrement = 1024;
        const char *bufEnd;
        char *buf, *bptr, *eptr, *pptr;

        enum ParseState {
            FindStart, GetCommand, GetToken, GetAttribute, GetQuote, GetValue
        };
        ParseState parseState;

        const char *commandPtr, *attrName;
        char *attrValue;
        char quote;
        Attr attr;
};

}
#endif //MSRINBUF_H
