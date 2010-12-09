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
const size_t Signal::dataTypeIndex[HRTLab::Variable::maxWidth+1] = {
    3 /*0*/, 3 /*1*/, 2 /*2*/, 3 /*3*/,
    1 /*4*/, 3 /*5*/, 3 /*6*/, 3 /*7*/, 0 /*8*/
};

//////////////////////////////////////////////////////////////////////
Signal::Signal(Main *main,
        const Task *task,
        size_t index,
        unsigned int decimation,
        const char *path,
        enum si_datatype_t dtype,
        const void *addr,
        unsigned int ndims,
        const size_t *dim):
    HRTLab::Signal(main, task, decimation, path, dtype, ndims, dim),
    index(index), addr(addr), task(task),
    subscriptionIndex(dataTypeIndex[width])
{
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

//////////////////////////////////////////////////////////////////////
void Signal::getValue(char *buf, struct timespec *t) const
{
    const HRTLab::Signal *s = this;
    main->getValues(&s, 1, buf, t);
}
