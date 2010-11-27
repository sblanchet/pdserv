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
Parameter::Parameter( unsigned int index,
        const char *path,
        unsigned int mode,
        enum si_datatype_t dtype,
        void *addr,
        unsigned int ndims,
        const size_t *dim):
    Variable(path, dtype, ndims, dim),
    index(index), addr(addr),
    mode(mode)
{
    trigger = copy;
}

//////////////////////////////////////////////////////////////////////
int Parameter::copy(unsigned int tid,
        void *dst, const void *src, size_t len, void *)
{
    std::copy( reinterpret_cast<const char*>(src), 
            reinterpret_cast<const char*>(src)+len,
            reinterpret_cast<char*>(dst));
    return 0;
}

//////////////////////////////////////////////////////////////////////
int Parameter::setValue_(unsigned int tid, const char *valbuf) const
{
    return trigger(tid, addr, valbuf, memSize, priv_data);
}

//////////////////////////////////////////////////////////////////////
void Parameter::setTrigger(paramtrigger_t t, void *d)
{
    trigger = t ? t : copy;
    priv_data = d;
}
