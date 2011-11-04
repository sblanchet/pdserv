/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdserv package.
 *
 *  pdserv is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pdserv is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pdserv. See COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include "Outbuf.h"
#include "Session.h"

#include <locale>

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Outbuf::Outbuf(Session *s): std::ostream(this), session(s)
{
    bptr = eptr = pptr = 0;
    free = 0;

    std::ostream::imbue(std::locale::classic());
}

/////////////////////////////////////////////////////////////////////////////
Outbuf::~Outbuf()
{
    delete[] bptr;
}

/////////////////////////////////////////////////////////////////////////////
const char *Outbuf::bufptr() const
{
    return pptr;
}

/////////////////////////////////////////////////////////////////////////////
size_t Outbuf::size() const
{
    return eptr - pptr;
}

/////////////////////////////////////////////////////////////////////////////
bool Outbuf::clear(size_t n)
{
    pptr += n;

    if (pptr == eptr) {
        free += eptr - bptr;
        pptr = eptr = bptr;
    }

    return pptr == eptr;
}

/////////////////////////////////////////////////////////////////////////////
int Outbuf::sync()
{
    if (pptr != eptr)
        session->requestOutput();
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
int Outbuf::overflow(int c)
{
    checkFreeSpace(1);
    *eptr++ = c;
    free--;
    return c;
}

/////////////////////////////////////////////////////////////////////////////
std::streamsize Outbuf::xsputn ( const char * s, std::streamsize n )
{
    checkFreeSpace(n);
    std::copy(s, s+n, eptr);
    eptr += n;
    free -= n;
    return n;
}

/////////////////////////////////////////////////////////////////////////////
void Outbuf::checkFreeSpace(size_t n)
{
    static size_t chunk = 1024 - 1;

    // Check whether there is enough space at the end of the buffer
    if (free >= n)
        return;

    // Check whether total free space is enough
    if (free + (pptr - bptr) >= n) {
        std::copy(pptr, eptr, bptr);
        eptr -= pptr - bptr;
        free += pptr - bptr;
        pptr = bptr;
        return;
    }

    // Increment in chunk quantums
    free += (n + chunk) & ~chunk;
    char *p = new char[free + (eptr - bptr)];

    // Copy unwritten data to new buffer
    std::copy(pptr, eptr, p);

    delete[] bptr;

    // Update pointers
    eptr = p + (eptr - pptr);
    free += pptr - bptr;
    bptr = pptr = p;
}
