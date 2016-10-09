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

#include "Session.h"
#include "Main.h"
#include "Debug.h"

#include <cerrno>
#include <cstring>      // strerror()
#include <log4cplus/logger.h>

using namespace PdServ;

/////////////////////////////////////////////////////////////////////////////
Session::Session(const Main *m, log4cplus::Logger& log, size_t bufsize)
: main(m), log(log)
{
    p_eof = false;

    main->gettime(&connectedTime);
    main->prepare(this);
    eventId = ~0U;
    main->getNextEvent(this);

    char* buf = new char[bufsize];
    setp(buf, buf + bufsize);
}

/////////////////////////////////////////////////////////////////////////////
Session::~Session()
{
    main->cleanup(this);
    delete[] pbase();
}

/////////////////////////////////////////////////////////////////////////////
bool Session::eof() const
{
    return p_eof;
}

/////////////////////////////////////////////////////////////////////////////
int Session::overflow(int value)
{
    char c = value;
    return xsputn(&c, 1) ? c : traits_type::eof();
}

/////////////////////////////////////////////////////////////////////////////
std::streamsize Session::xsputn(const char * buf, std::streamsize count)
{
    std::streamsize rest = count;
    do {
        // Put data into buffer
        size_t n = std::min(epptr() - pptr(), rest);
        std::copy(buf, buf + n, pptr());

        // Update pointers
        pbump(n);
        buf += n;
        rest -= n;

        // flush if buffer is full
        if ((pptr() == epptr() and sync()) or p_eof)
            return 0;

    } while (rest);

    return count;
}

/////////////////////////////////////////////////////////////////////////////
int Session::sync()
{
    size_t count = pptr() - pbase();
    if (!count)
        return 0;

    ssize_t result = write(pbase(), count);
    if (result > 0) {
        std::copy(pbase() + result, pptr(), pbase());
        pbump(-result);
        return 0;
    }

    // write() unsuccessful. Filter out EINTR and EAGAIN
    if (result != -EINTR and result != -EAGAIN) {
        p_eof = true;

        if (result)
            LOG4CPLUS_ERROR(log,
                    LOG4CPLUS_TEXT("Network error: ")
                    << LOG4CPLUS_C_STR_TO_TSTRING(strerror(-result)));
        else
            LOG4CPLUS_INFO_STR(log,
                    LOG4CPLUS_TEXT("Client closed connection"));
    }

    return result;
}

/////////////////////////////////////////////////////////////////////////////
int Session::underflow()
{
    char c;
    return xsgetn(&c, 1) == 1 ? c : traits_type::eof();
}

/////////////////////////////////////////////////////////////////////////////
std::streamsize Session::xsgetn(char* buf, std::streamsize count)
{
    ssize_t result= read(buf, count);
    if (result > 0)
        return result;

    // read() unsuccessful. Filter out EINTR and EAGAIN
    if (result != -EINTR and result != -EAGAIN) {
        p_eof = true;

        if (result)
            LOG4CPLUS_ERROR(log,
                    LOG4CPLUS_TEXT("Network error: ")
                    << LOG4CPLUS_C_STR_TO_TSTRING(strerror(-result)));
        else
            LOG4CPLUS_INFO_STR(log,
                    LOG4CPLUS_TEXT("Client closed connection"));
    }

    return 0;
}
