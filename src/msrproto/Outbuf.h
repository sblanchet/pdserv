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

#ifndef MSROUTBUF_H
#define MSROUTBUF_H

#include <streambuf>
#include <ostream>

namespace MsrProto {

class Session;

class Outbuf: public std::streambuf, public std::ostream {
    public:
        Outbuf(Session *);
        ~Outbuf();

        const char *bufptr() const;
        size_t size() const;

        bool clear(size_t n);

    private:
        Session * const session;

        // Output buffer management
        // Somewhat elusive, but makes writing to end of buffer and sending
        // data fast
        char *bptr;     // Beginning of buffer
        char *pptr;     // Beginning of output data
        char *eptr;     // End of output data
        size_t free;    // Free space between end of data till buffer end
        void checkFreeSpace(size_t n);       // Check for free space

        // Reimplemented from streambuf
        int sync();
        int overflow(int c);
        std::streamsize xsputn ( const char * s, std::streamsize n );
};

}
#endif //MSROUTBUF_H
