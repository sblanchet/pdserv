/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include <sys/time.h>
#include <ctime>

#include "Parameter.h"

//#include <iostream>
//using std::cout;
//using std::cerr;
//using std::endl;

using namespace HRTLab;

//////////////////////////////////////////////////////////////////////
Parameter::Parameter(
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
    paramcheck(paramcheck),
    paramupdate(paramupdate),
    priv_data(priv_data),
    addr(addr)
{
    struct timeval t;
    if (!gettimeofday(&t, 0)) {
        mtime.tv_sec = t.tv_sec;
        mtime.tv_nsec = t.tv_usec * 1000;
    }
}

//////////////////////////////////////////////////////////////////////
const timespec& Parameter::getMtime() const
{
    return mtime;
}
