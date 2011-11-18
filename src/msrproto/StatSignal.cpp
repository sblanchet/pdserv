/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdserv package.
 *
 *  pdserv is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pdserv is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pdserv. See COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include "StatSignal.h"
#include "Session.h"
#include "TaskStatistics.h"
#include "../SessionTaskData.h"
#include "../TaskStatistics.h"
#include "../Main.h"
#include "../Task.h"

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
StatSignal::StatSignal(const PdServ::Task *t, Type type):
    PdServ::Signal("", t->sampleTime, si_uint32_T, 1, 0),
    task(t), type(type)
{
}

/////////////////////////////////////////////////////////////////////////////
void StatSignal::subscribe(PdServ::Session *session) const
{
    const PdServ::Signal *signal = this;
    session->newSignalList(task, &signal, 1);
}

/////////////////////////////////////////////////////////////////////////////
void StatSignal::unsubscribe(PdServ::Session *) const
{
}

/////////////////////////////////////////////////////////////////////////////
const unsigned int *StatSignal::getAddr(
        const PdServ::Session *session, struct timespec *t) const
{
    const TaskStatistics& stats =
        static_cast<const Session*>(session)->getTaskStatistics(task);

    if (t)
        *t = stats.taskStatistics->time;

    switch (type) {
        case ExecTime:
            return &stats.execTime;
            break;

        case Period:
            return &stats.cycleTime;
            break;

        case Overrun:
            return &stats.taskStatistics->overrun;
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
double StatSignal::poll(const PdServ::Session *session,
        void *buf, struct timespec *t) const
{
    *reinterpret_cast<uint32_t*>(buf) = *getAddr(session, t);

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
const void *StatSignal::getValue(const PdServ::SessionTaskData* std) const
{
//    cout << __PRETTY_FUNCTION__ << endl;
    return getAddr(std->session, 0);
}

/////////////////////////////////////////////////////////////////////////////
void StatSignal::getValue(
        PdServ::Session *session, void *buf, struct timespec* t) const
{
//    cout << __PRETTY_FUNCTION__ << endl;
    poll(session, buf, t);
}
