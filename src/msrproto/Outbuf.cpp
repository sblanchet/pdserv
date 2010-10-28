/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "Outbuf.h"
#include "Session.h"

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Outbuf::Outbuf(Session *s): std::ostream(this), session(s)
{
    wbuf = wbufeptr = wbufpptr = 0;
    wbuffree = 0;
}

/////////////////////////////////////////////////////////////////////////////
Outbuf::~Outbuf()
{
    delete[] wbuf;
}

/////////////////////////////////////////////////////////////////////////////
const char *Outbuf::bufptr() const
{
    return wbufpptr;
}

/////////////////////////////////////////////////////////////////////////////
size_t Outbuf::size() const
{
    return wbufeptr - wbufpptr;
}

/////////////////////////////////////////////////////////////////////////////
bool Outbuf::clear(size_t n)
{
    wbufpptr += n;

    if (wbufpptr == wbufeptr) {
        wbuffree += wbufeptr - wbuf;
        wbufpptr = wbufeptr = wbuf;
    }

    return wbufpptr == wbufeptr;
}

/////////////////////////////////////////////////////////////////////////////
int Outbuf::sync()
{
    if (wbufpptr != wbufeptr)
        session->requestOutput();
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
int Outbuf::overflow(int c)
{
    checkwbuf(1);
    *wbufeptr++ = c;
    wbuffree--;
    return c;
}

/////////////////////////////////////////////////////////////////////////////
std::streamsize Outbuf::xsputn ( const char * s, std::streamsize n )
{
    checkwbuf(n);
    std::copy(s, s+n, wbufeptr);
    wbufeptr += n;
    wbuffree -= n;
    return n;
}

/////////////////////////////////////////////////////////////////////////////
void Outbuf::checkwbuf(size_t n)
{
    static size_t chunk = 1024 - 1;

    // Check whether there is enough space at the end of the buffer
    if (wbuffree >= n)
        return;

    // Check whether total free space is enough
    if (wbuffree + (wbufpptr - wbuf) >= n) {
        std::copy(wbufpptr, wbufeptr, wbuf);
        wbufeptr -= wbufpptr - wbuf;
        wbuffree += wbufpptr - wbuf;
        wbufpptr = wbuf;
        return;
    }

    // Increment in chunk quantums
    wbuffree += (n + chunk) & ~chunk;
    char *p = new char[wbuffree + (wbufeptr - wbuf)];

    // Copy unwritten data to new buffer
    std::copy(wbufpptr, wbufeptr, p);

    delete[] wbuf;

    // Update pointers
    wbufeptr = p + (wbufeptr - wbufpptr);
    wbuffree += wbufpptr - wbuf;
    wbuf = wbufpptr = p;
}
