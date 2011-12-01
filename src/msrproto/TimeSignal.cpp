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

#include "TimeSignal.h"
#include "Session.h"
#include "TaskStatistics.h"
#include "../SessionTaskData.h"
#include "../TaskStatistics.h"
#include "../Main.h"
#include "../Task.h"

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
TimeSignal::TimeSignal(const PdServ::Task *t, const std::string& path):
    PdServ::Signal(path, t->sampleTime, double_T, 1, 0),
    task(t)
{
}

/////////////////////////////////////////////////////////////////////////////
void TimeSignal::subscribe(PdServ::Session *session) const
{
    const PdServ::Signal *signal = this;
    session->newSignalList(task, &signal, 1);
}

/////////////////////////////////////////////////////////////////////////////
void TimeSignal::unsubscribe(PdServ::Session *) const
{
}

/////////////////////////////////////////////////////////////////////////////
double TimeSignal::poll(const PdServ::Session *session,
        void *buf, struct timespec *t) const
{
    const PdServ::TaskStatistics *stats =
        static_cast<const Session*>(session)->getTaskStatistics(task);

    *reinterpret_cast<double*>(buf) =
        1.0e-9 * stats->time.tv_nsec + stats->time.tv_sec;

    if (t)
        *t = stats->time;

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
const void *TimeSignal::getValue(const PdServ::SessionTaskData* std) const
{
    double* value =
        static_cast<const Session*>(std->session)->getDouble();

    poll(std->session, value, 0);

    return value;
}

/////////////////////////////////////////////////////////////////////////////
void TimeSignal::getValue(const PdServ::Session *session,
        void *buf, size_t start, size_t count, struct timespec* t) const
{
    poll(session, buf, t);
}
