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

#include "Signal.h"

#include "../Session.h"
#include "../Debug.h"
#include "Main.h"
#include "Task.h"
#include "SessionTaskData.h"

//////////////////////////////////////////////////////////////////////
Signal::Signal( const Task *task, const SignalInfo& si):
    PdServ::Signal(si.path(), task->sampleTime,
            si.dataType(), si.ndim(), si.getDim()),
    main(task->main), offset(si.si->offset), info(si), task(task)
{
}

//////////////////////////////////////////////////////////////////////
void Signal::subscribe(PdServ::Session *session) const
{
    session->newSignal(task, this);
}

//////////////////////////////////////////////////////////////////////
void Signal::unsubscribe(PdServ::Session *) const
{
}

//////////////////////////////////////////////////////////////////////
double Signal::poll(const PdServ::Session *, void *, struct timespec *) const
{
    return 0;
}

//////////////////////////////////////////////////////////////////////
void Signal::getValue( const PdServ::Session *, void *dest,
        struct timespec *t) const
{
    const PdServ::Signal *signal = this;

    main->poll(0, &signal, 1, dest, t);
}

//////////////////////////////////////////////////////////////////////
const void *Signal::getValue(const PdServ::SessionTaskData *std) const
{
    return static_cast<const SessionTaskData*>(std)->getValue(this);
}
