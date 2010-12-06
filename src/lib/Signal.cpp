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
const size_t Signal::index[HRTLab::Variable::maxWidth+1] = {
    3 /*0*/, 3 /*1*/, 2 /*2*/, 3 /*3*/,
    1 /*4*/, 3 /*5*/, 3 /*6*/, 3 /*7*/, 0 /*8*/
};

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
    addr(addr), task(task), subscriptionIndex(index[width]), mutex(1)
{
    //cout << __func__ << " addr = " << addr << endl;
}

//////////////////////////////////////////////////////////////////////
void Signal::subscribe(const HRTLab::Session *session) const
{
    ost::SemaphoreLock lock(mutex);

    const Signal *s = this;
    if (sessions.empty())
        task->subscribe(&s, 1);

    sessions.insert(session);
}

//////////////////////////////////////////////////////////////////////
void Signal::unsubscribe(const HRTLab::Session *session) const
{
    ost::SemaphoreLock lock(mutex);

    sessions.erase(session);

    const Signal *s = this;
    if (sessions.empty())
        task->unsubscribe(&s, 1);
}

//////////////////////////////////////////////////////////////////////
void Signal::getValue(char *buf, struct timespec *t) const
{
    const HRTLab::Signal *s = this;
    main->getValues(&s, 1, buf);
}
