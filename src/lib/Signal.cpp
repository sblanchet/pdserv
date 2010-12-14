/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "config.h"

#include "Signal.h"
#include "Task.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

//////////////////////////////////////////////////////////////////////
const size_t Signal::dataTypeIndex[HRTLab::Variable::maxWidth+1] = {
    3 /*0*/, 3 /*1*/, 2 /*2*/, 3 /*3*/,
    1 /*4*/, 3 /*5*/, 3 /*6*/, 3 /*7*/, 0 /*8*/
};

//////////////////////////////////////////////////////////////////////
Signal::Signal( Task *task,
        unsigned int decimation,
        const char *path,
        enum si_datatype_t dtype,
        const void *addr,
        unsigned int ndims,
        const size_t *dim):
    HRTLab::Signal(task, decimation, path, dtype, ndims, dim),
    addr(reinterpret_cast<const char *>(addr)),
    task(task), subscriptionIndex(dataTypeIndex[width])
{
    task->newSignal(this);
    //cout << __func__ << " addr = " << addr << endl;
}

//////////////////////////////////////////////////////////////////////
void Signal::subscribe(const HRTLab::Session *session) const
{
    const HRTLab::Signal *s = this;
    task->subscribe(session, &s, 1);
}

//////////////////////////////////////////////////////////////////////
void Signal::unsubscribe(const HRTLab::Session *session) const
{
    const HRTLab::Signal *s = this;
    task->unsubscribe(session, &s, 1);
}
