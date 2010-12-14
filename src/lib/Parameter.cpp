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
    addr(reinterpret_cast<char*>(addr)), mutex(1)
{
    trigger = copy;

    mtime.tv_sec = 0;
    mtime.tv_nsec = 0;

    valueBuf = new char[memSize];
}

//////////////////////////////////////////////////////////////////////
Parameter::~Parameter()
{
    delete[] valueBuf;
}

//////////////////////////////////////////////////////////////////////
void Parameter::copyValue(const char* src, const struct timespec& time) const
{
    ost::SemaphoreLock lock(mutex);

    std::copy(src, src + memSize, valueBuf);
    mtime = time;
}

//////////////////////////////////////////////////////////////////////
void Parameter::getValue(char* dst, struct timespec *time) const
{
    ost::SemaphoreLock lock(mutex);
    std::copy(valueBuf, valueBuf + memSize, dst);
    if (time)
        *time = mtime;
}

//////////////////////////////////////////////////////////////////////
int Parameter::copy(unsigned int tid, char checkOnly,
        void *dst, const void *src, size_t len, void *)
{
//    cout << __PRETTY_FUNCTION__ << checkOnly << endl;
    if (checkOnly)
        return 0;

    std::copy( reinterpret_cast<const char*>(src), 
            reinterpret_cast<const char*>(src)+len,
            reinterpret_cast<char*>(dst));

    return 0;
}
