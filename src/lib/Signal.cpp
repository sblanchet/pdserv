/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "config.h"

#include "Signal.h"
#include "Task.h"
#include "Main.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

//////////////////////////////////////////////////////////////////////
Signal::Signal(Main *main,
        const Task *task,
        unsigned int decimation,
        const char *path,
        enum si_datatype_t dtype,
        const void *addr,
        unsigned int ndims,
        const size_t *dim):
    HRTLab::Signal(main, task, decimation, path, dtype, ndims, dim),
    addr(addr), task(task), mutex(1)
{
}

//////////////////////////////////////////////////////////////////////
void Signal::subscribe(const HRTLab::Session *session)
{
    ost::SemaphoreLock lock(mutex);

    if (sessions.empty())
        task->subscribe(this);

    sessions.insert(session);
}

//////////////////////////////////////////////////////////////////////
void Signal::unsubscribe(const HRTLab::Session *session)
{
    ost::SemaphoreLock lock(mutex);

    sessions.erase(session);

    if (sessions.empty())
        task->unsubscribe(this);
}

//////////////////////////////////////////////////////////////////////
void Signal::getValue(char *buf, struct timespec *t) const
{
    task->poll(this, buf, t);
}
