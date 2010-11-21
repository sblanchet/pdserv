/*****************************************************************************
 * $Id$
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
