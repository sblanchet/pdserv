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

#include "config.h"
#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

#include "SessionMirror.h"
#include "SessionTaskData.h"
#include "Task.h"
#include "Main.h"

////////////////////////////////////////////////////////////////////////////
SessionMirror::SessionMirror(const Main *main, PdServ::Session* session):
    main(main), session(session)
{
    cout << __func__ << endl;
    for (size_t i = 0; i < main->numTasks(); ++i)
        taskSet.insert(new SessionTaskData( session, main->getTask(i)));
}

////////////////////////////////////////////////////////////////////////////
SessionMirror::~SessionMirror()
{
    cout << __func__ << endl;
    for (TaskSet::iterator it = taskSet.begin(); it != taskSet.end(); ++it)
        delete *it;
}

////////////////////////////////////////////////////////////////////////////
bool SessionMirror::rxPdo()
{
    bool error = false;
    for (TaskSet::iterator it = taskSet.begin(); it != taskSet.end(); ++it)
        error |= (*it)->rxPdo();

    return error;
}
