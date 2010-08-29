/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include <sys/time.h>
#include <ctime>
#include <algorithm>

#include "Parameter.h"
#include "Main.h"

//#include <iostream>
//using std::cout;
//using std::cerr;
//using std::endl;

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
    priv_data(priv_data),
    addr(addr)
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
const timespec& Parameter::getMtime() const
{
    return mtime;
}

//////////////////////////////////////////////////////////////////////
void Parameter::setValue(const char *valbuf, size_t buflen, size_t offset)
{
    if (!buflen) {
        paramupdate(addr, valbuf, memSize, priv_data);
        return;
    }
    
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

    main->writeParameter(this);
}
