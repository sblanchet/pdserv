/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "config.h"

#include <algorithm>

#include "Parameter.h"
#include "Main.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

//////////////////////////////////////////////////////////////////////
Parameter::Parameter(
        Main *main,
        const char *path,
        unsigned int mode,
        enum si_datatype_t dtype,
        void *addr,
        unsigned int ndims,
        const size_t *dim):
    HRTLab::Parameter(main, path, mode, dtype, ndims, dim),
    addr(addr), main(main)
{
    trigger = copy;
}

//////////////////////////////////////////////////////////////////////
int Parameter::copy(unsigned int tid, char checkOnly,
        void *dst, const void *src, size_t len, void *)
{
    if (checkOnly)
        return 0;

    std::copy( reinterpret_cast<const char*>(src), 
            reinterpret_cast<const char*>(src)+len,
            reinterpret_cast<char*>(dst));
    return 0;
}

//////////////////////////////////////////////////////////////////////
int Parameter::setValue(const char *valbuf) const
{
    return main->setParameter(this, valbuf);
}

//////////////////////////////////////////////////////////////////////
const char * Parameter::getValue(struct timespec *t) const
{
    if (t)
        *t = mtime;

    return shmemAddr;
}
