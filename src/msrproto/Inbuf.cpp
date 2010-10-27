/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "Inbuf.h"

#include <algorithm>

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Inbuf::Inbuf()
{
    _bptr = 0;
    _eptr = 0;
    bufLen = 0;
}

/////////////////////////////////////////////////////////////////////////////
Inbuf::~Inbuf()
{
    delete[] _bptr;
}

/////////////////////////////////////////////////////////////////////////////
bool Inbuf::empty() const
{
    return _bptr == _eptr;
}

/////////////////////////////////////////////////////////////////////////////
void Inbuf::erase(const char *p)
{
    size_t n = p - _bptr;
    std::copy(_bptr + n, _eptr, _bptr);
    _eptr -= n;
}

/////////////////////////////////////////////////////////////////////////////
char *Inbuf::bptr() const
{
    return _bptr;
}

/////////////////////////////////////////////////////////////////////////////
const char *Inbuf::eptr() const
{
    return _eptr;
}

/////////////////////////////////////////////////////////////////////////////
char *Inbuf::rptr(size_t n)
{
    if (n) {
        _eptr += n;
        return 0;
    }

    if (!free()) {
        bufLen += 1024;

        char *newbuf = new char[bufLen];

        std::copy(_bptr, _eptr, newbuf);

        delete[] _bptr;

        _eptr = newbuf + (_eptr - _bptr);
        _bptr = newbuf;
    }

    return _eptr;
}

/////////////////////////////////////////////////////////////////////////////
size_t Inbuf::free() const
{
    return _bptr + bufLen - _eptr;
}
