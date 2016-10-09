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

#ifndef SESSION_H
#define SESSION_H

#include <ctime>
#include <streambuf>
#include <unistd.h>

struct SessionData;

namespace log4cplus {
    class Logger;
}

namespace PdServ {

class Main;
class Event;

class Session: public std::streambuf {
    public:
        Session(const Main *main, 
                log4cplus::Logger& log,
                size_t bufsize = 4096);
        virtual ~Session();

        const Main * const main;
        size_t eventId;

        bool eof() const;

    protected:
        struct timespec connectedTime;

        virtual ssize_t write(const void* buf, size_t len) = 0;
        virtual ssize_t read(       void* buf, size_t len) = 0;

    private:
        bool p_eof;

        log4cplus::Logger& log;

        // Reimplemented from std::streambuf
        int overflow(int c);
        std::streamsize xsputn(const char* s, std::streamsize n);
        int sync();

        int underflow();
        std::streamsize xsgetn(char* s, std::streamsize n);
};

}
#endif //SESSION_H
