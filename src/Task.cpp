/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "Task.h"

using namespace HRTLab;

/////////////////////////////////////////////////////////////////////////////
Task::Task(const Main *m, unsigned int tid, double sampleTime):
    tid(tid), sampleTime(sampleTime), main(m)
{
}
