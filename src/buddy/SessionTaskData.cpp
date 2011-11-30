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

#include "../Session.h"
#include "../Debug.h"
#include "SessionTaskData.h"
#include "Task.h"
#include "Signal.h"
#include <app_taskstats.h>
#include <fio_ioctl.h>

////////////////////////////////////////////////////////////////////////////
SessionTaskData::SessionTaskData( PdServ::Session* s, const Task* t,
        const char *album, unsigned int current,
        const struct app_properties *app_properties):
    PdServ::SessionTaskData(s, t),
    album(album), photoSize(app_properties->rtB_size),
    statsOffset(photoSize
            - app_properties->num_tasks*sizeof(struct task_stats))
{
    photo = album + current*photoSize;
    updateStatistics();
}

////////////////////////////////////////////////////////////////////////////
const PdServ::TaskStatistics* SessionTaskData::getTaskStatistics(
        const Task*) const
{
    debug();
    return &stat;
}

////////////////////////////////////////////////////////////////////////////
void SessionTaskData::updateStatistics()
{
    const struct task_stats *task_stats =
        reinterpret_cast<const struct task_stats*>(photo + statsOffset);

    stat.seqNo++;

#ifdef TIMESPEC
    stat.time = task_stats[0].time;
#else
    stat.time.tv_sec = task_stats[0].time.tv_sec;
    stat.time.tv_nsec = 1000*task_stats[0].time.tv_usec;
#endif

    stat.exec_time = 1.0e-6 * task_stats[0].exec_time;
    stat.cycle_time = 1.0e-6 * task_stats[0].time_step;
    stat.overrun = task_stats[0].overrun;
}

////////////////////////////////////////////////////////////////////////////
void SessionTaskData::newSignalData(unsigned int current)
{
    photo = album + current*photoSize;

    updateStatistics();

    session->newSignalData(this);
}

////////////////////////////////////////////////////////////////////////////
const void *SessionTaskData::getValue(const Signal *signal) const
{
    return photo + signal->offset;
}
