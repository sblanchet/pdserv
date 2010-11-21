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
        unsigned int index,
        const char *path,
        const char *alias,
        enum si_datatype_t dtype,
        unsigned int ndims,
        const size_t dim[],
        char *addr,
        paramupdate_t paramcopy,
        void *priv_data) :
    Variable(index, path, alias, dtype, ndims, dim, 0, addr),
    paramcopy(paramcopy ? paramcopy : copy),
    priv_data(priv_data), addr(addr)
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
int Parameter::setValue(const char *valbuf) const
{
    return paramcopy(addr, valbuf, memSize, priv_data);
}
