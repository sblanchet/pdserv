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

#include "SessionShadow.h"
#include "SessionTaskData.h"
#include "Task.h"
#include "Main.h"

////////////////////////////////////////////////////////////////////////////
SessionShadow::SessionShadow(const Main *main, PdServ::Session* session):
    main(main), session(session)
{
    for (size_t i = 0; i < main->numTasks(); ++i) {
        Task *task = main->getTask(i);
        taskMap[task] = new SessionTaskData( session, task);
    }
}

////////////////////////////////////////////////////////////////////////////
SessionShadow::~SessionShadow()
{
    for (TaskMap::iterator it = taskMap.begin(); it != taskMap.end(); ++it)
        delete it->second;
}

////////////////////////////////////////////////////////////////////////////
bool SessionShadow::rxPdo()
{
    bool error = false;
    for (TaskMap::iterator it = taskMap.begin(); it != taskMap.end(); ++it)
        error |= it->second->rxPdo();

    return error;
}

////////////////////////////////////////////////////////////////////////////
const PdServ::TaskStatistics *SessionShadow::getTaskStatistics(
        const PdServ::Task *task) const
{
    return taskMap.find(task)->second->getTaskStatistics();
}
