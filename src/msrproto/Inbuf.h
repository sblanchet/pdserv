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
