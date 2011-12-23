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
StatSignal::StatSignal(const PdServ::Task *t,
        const std::string& path, Type type):
    PdServ::Signal(path, t->sampleTime, double_T, 1, 0),
    task(t), type(type)
{
}

/////////////////////////////////////////////////////////////////////////////
void StatSignal::subscribe(PdServ::Session *session) const
{
    const PdServ::Signal *signal = this;
    session->newSignal(task, signal);
}

/////////////////////////////////////////////////////////////////////////////
void StatSignal::unsubscribe(PdServ::Session *) const
{
}

/////////////////////////////////////////////////////////////////////////////
double StatSignal::getValue(
        const PdServ::Session *session, struct timespec *t) const
{
    const PdServ::TaskStatistics* stats = session->getTaskStatistics(task);

    if (t)
        *t = *session->getTaskTime(task);

    switch (type) {
        case ExecTime:
            return stats->exec_time;
            break;

        case Period:
            return stats->cycle_time;
            break;

        case Overrun:
            return stats->overrun;
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
double StatSignal::poll(const PdServ::Session *session,
        void *buf, struct timespec *t) const
{
    *reinterpret_cast<double*>(buf) = getValue(session, t);

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
const void *StatSignal::getValue(const PdServ::SessionTaskData* std) const
{
//    cout << __PRETTY_FUNCTION__ << endl;
    double* value =
        static_cast<const Session*>(std->session)->getDouble();
    *value = getValue(std->session, 0);
    return value;
}

/////////////////////////////////////////////////////////////////////////////
void StatSignal::getValue( const PdServ::Session *session,
        void *buf, struct timespec* t) const
{
//    cout << __PRETTY_FUNCTION__ << endl;
    poll(session, buf, t);
}
