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
        size_t index,
        const char *path,
        unsigned int mode,
        enum si_datatype_t dtype,
        void *addr,
        unsigned int ndims,
        const size_t *dim):
    HRTLab::Parameter(main, path, mode, dtype, ndims, dim),
    index(index), addr(addr), main(main)
{
    mtime.tv_sec = 0;
    mtime.tv_nsec = 0;

    trigger = copy;
}

//////////////////////////////////////////////////////////////////////
Parameter::~Parameter()
{
}

//////////////////////////////////////////////////////////////////////
int Parameter::copy(unsigned int tid, char checkOnly,
        void *dst, const void *src, size_t len, void *)
{
    cout << __PRETTY_FUNCTION__ << checkOnly << endl;
    if (checkOnly)
        return 0;

    std::copy( reinterpret_cast<const char*>(src), 
            reinterpret_cast<const char*>(src)+len,
            reinterpret_cast<char*>(dst));
    return 0;
}

//////////////////////////////////////////////////////////////////////
int Parameter::setValue(const char *valbuf)
{
    HRTLab::Parameter *p = this;
    return
        static_cast<const HRTLab::Main*>(main)->setParameters(&p, 1, valbuf);
}

//////////////////////////////////////////////////////////////////////
void Parameter::getValue(char *buf, struct timespec* t) const
{
    const HRTLab::Parameter *p = this;
    static_cast<const HRTLab::Main*>(main)->getValues(&p, 1, buf, t);
}
