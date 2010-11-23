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
        unsigned int ndims,
        const size_t *dim,
        const char *addr):
    Variable(index, path, dtype, ndims, dim,
            decimation ? decimation : 1, addr),
    tid(tid)
{
//    cout << __func__ << index << endl;
}
