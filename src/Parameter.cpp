/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "config.h"

#include <ctime>
#include <algorithm>

#include "Parameter.h"
#include "Main.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace HRTLab;

//////////////////////////////////////////////////////////////////////
Parameter::Parameter(
        Main *main,
        unsigned int index,
        const char *path,
        const char *alias,
        enum si_datatype_t dtype,
        unsigned int ndims,
        const size_t dim[],
        char *addr,
        paramupdate_t paramcheck,
        paramupdate_t paramupdate,
        void *priv_data) :
    Variable(index, path, alias, dtype, ndims, dim, 0, addr),
    main(main),
    paramcheck(paramcheck ? paramcheck : copy),
    paramupdate(paramupdate ? paramupdate : copy),
    priv_data(priv_data), addr(addr), mutex(1)
{
}

//////////////////////////////////////////////////////////////////////
int Parameter::copy(void *dst, const void *src, size_t len, void *)
{
    std::copy( reinterpret_cast<const char*>(src), 
            reinterpret_cast<const char*>(src)+len,
            reinterpret_cast<char*>(dst));
    return 0;
}

//////////////////////////////////////////////////////////////////////
struct timespec Parameter::getMtime() const
{
    ost::SemaphoreLock wait(mutex);
    return mtime;
}

//////////////////////////////////////////////////////////////////////
void Parameter::setValue(const char *valbuf, size_t nelem, size_t offset)
{
    size_t buflen = nelem * width;

    // If nelem == 0, this method was called from inside the real-time thread.
    // In this case, call the paramupdate callback which copies the parameter
    // from shared memory to the process.
    if (!buflen) {
        paramupdate(addr, valbuf, memSize, priv_data);
        return;
    }

    ost::SemaphoreLock wait(mutex);

    char buf[memSize];

    if (offset or buflen != memSize) {
        if (buflen + offset*width > memSize)
            return;

        std::copy(addr, addr + memSize, buf);
        std::copy(valbuf, valbuf + buflen, buf + offset*width);

        valbuf = buf;
    }

    if (paramcheck(addr, valbuf, memSize, priv_data))
        return;

    main->gettime(&mtime);

//    main->writeParameter(this);
}
