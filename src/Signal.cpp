/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "Signal.h"

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

using namespace HRTLab;

//////////////////////////////////////////////////////////////////////
Signal::Signal( unsigned int tid,
        unsigned int index,
        unsigned int decimation,
        const char *path,
        const char *alias,
        enum si_datatype_t dtype,
        unsigned int ndims,
        const size_t *dim,
        const char *addr):
    Variable(index, path, alias, dtype, ndims, dim,
            decimation ? decimation : 1, addr),
    tid(tid)
{
    cout << __func__ << index << endl;
}
