/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdserv library.
 *
 *  The pdserv library is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or (at
 *  your option) any later version.
 *
 *  The pdserv library is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with the pdserv library. If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include "SessionShadow.h"
#include "Task.h"
#include "../Session.h"
#include "../TaskStatistics.h"
#include "../Debug.h"
#include <fio_ioctl.h>

////////////////////////////////////////////////////////////////////////////
SessionShadow::SessionShadow(PdServ::Session *session,
        const Task *task, unsigned int current,
        const unsigned int* photoReady, const char *album,
        const struct app_properties *app_properties):
    taskData(session, task, album, current, app_properties),
    current(current), photoReady(photoReady), count(app_properties->rtB_count)
{
    lastPhoto = photoReady[current] - 1;
}

////////////////////////////////////////////////////////////////////////////
SessionShadow::~SessionShadow()
{
}

////////////////////////////////////////////////////////////////////////////
bool SessionShadow::rxPdo()
{
    while (static_cast<int>(photoReady[current] - lastPhoto) > 0) {
        taskData.newSignalData(current);

        lastPhoto = photoReady[current];
        if (++current == count)
            current = 0;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////
const PdServ::TaskStatistics *SessionShadow::getTaskStatistics(
        const PdServ::Task *task) const
{
    return taskData.getTaskStatistics(static_cast<const Task*>(task));
}

////////////////////////////////////////////////////////////////////////////
const timespec *SessionShadow::getTaskTime(
        const PdServ::Task *task) const
{
    return taskData.getTaskTime(static_cast<const Task*>(task));
}
