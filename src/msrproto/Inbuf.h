/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef MSRINBUF_H
#define MSRINBUF_H

#include <stdint.h>
#include <cstddef>

namespace MsrProto {

class Inbuf {
    public:
        Inbuf();
        ~Inbuf();
        bool empty() const;
        void erase(const char *p);
        char *bptr() const;
        const char *eptr() const;
        char *rptr(size_t n = 0);
        size_t free() const;

    private:
        size_t bufLen;
        char *_bptr, *_eptr;
};

}
#endif //MSRINBUF_H
