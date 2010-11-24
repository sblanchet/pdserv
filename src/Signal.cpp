/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "config.h"

#include "Signal.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace HRTLab;

//////////////////////////////////////////////////////////////////////
Signal::Signal( unsigned int tid,
        unsigned int index,
        unsigned int decimation,
        const char *path,
        enum si_datatype_t dtype,
        const void *addr,
        unsigned int ndims,
        const size_t *dim):
    Variable(index, path, dtype, addr, ndims, dim),
    tid(tid), decimation(decimation)
{
//    cout << __func__ << index << endl;
}
