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
        unsigned int decimation,
        const char *path,
        const char *alias,
        enum si_datatype_t dtype,
        unsigned int ndims,
        const size_t *dim,
        const void *addr):
    Variable(path, alias, dtype, ndims, dim, addr),
    tid(tid), signalDecimation(decimation)
{
    cout << __func__ << endl;
}
