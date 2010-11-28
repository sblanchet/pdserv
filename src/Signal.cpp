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
Signal::Signal( Task *task,
        unsigned int decimation,
        const char *path,
        enum si_datatype_t dtype,
        unsigned int ndims,
        const size_t *dim):
    Variable(path, dtype, ndims, dim),
    decimation(decimation), task(task)
{
//    cout << __func__ << index << endl;
}
