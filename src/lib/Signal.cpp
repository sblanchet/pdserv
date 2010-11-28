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
Signal::Signal( Task *task,
        unsigned int decimation,
        const char *path,
        enum si_datatype_t dtype,
        const void *addr,
        unsigned int ndims,
        const size_t *dim):
    HRTLab::Signal(task, decimation, path, dtype, ndims, dim),
    addr(addr), task(task)
{
}

//////////////////////////////////////////////////////////////////////
const char *Signal::getValue(const HRTLab::Session *session) const
{
    return task->getValue(session, this);
}

//////////////////////////////////////////////////////////////////////
void Signal::subscribe(HRTLab::Session *session) const
{
    task->subscribe(session, this);
}

//////////////////////////////////////////////////////////////////////
void Signal::unsubscribe(HRTLab::Session *session) const
{
    task->unsubscribe(session, this);
}

//////////////////////////////////////////////////////////////////////
void Signal::poll(char *buf) const
{
    task->poll(this, buf);
}
