/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "config.h"

#include "Signal.h"
#include "Main.h"
#include "Task.h"

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
    decimation(decimation), task(task) //,  main(main)
{
    task->main->newSignal(this);
}

//////////////////////////////////////////////////////////////////////
Signal::~Signal()
{
    //task->main->delSignal(this);
}

//////////////////////////////////////////////////////////////////////
void Signal::getValue(char *buf, struct timespec* t) const
{
    const Signal * s = this;
    task->main->getValues(&s, 1, buf, t);
}
