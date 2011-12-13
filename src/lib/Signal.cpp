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
#include "Signal.h"
#include "Task.h"
#include "Main.h"
#include "SessionTaskData.h"

//////////////////////////////////////////////////////////////////////
const size_t Signal::dataTypeIndex[PdServ::Variable::maxWidth+1] = {
    3 /*0*/, 3 /*1*/, 2 /*2*/, 3 /*3*/,
    1 /*4*/, 3 /*5*/, 3 /*6*/, 3 /*7*/, 0 /*8*/
};

//////////////////////////////////////////////////////////////////////
Signal::Signal( Task *task,
        size_t index,
        unsigned int decimation,
        const char *path,
        Datatype dtype,
        const void *addr,
        size_t ndims,
        const size_t *dim):
    PdServ::Signal(path, task->sampleTime * decimation, dtype, ndims, dim),
    addr(reinterpret_cast<const char *>(addr)),
    index(index), task(task),
    mutex(1)
{
}

//////////////////////////////////////////////////////////////////////
void Signal::subscribe(PdServ::Session *s) const
{
    ost::SemaphoreLock lock(mutex);

    if (sessions.empty())
        task->subscribe(this, true);
    else
        s->newSignal(task, this);

    sessions.insert(s);
}

//////////////////////////////////////////////////////////////////////
void Signal::unsubscribe(PdServ::Session *s) const
{
    ost::SemaphoreLock lock(mutex);

    sessions.erase(s);

    if (sessions.empty())
        task->subscribe(this, false);
}

//////////////////////////////////////////////////////////////////////
double Signal::poll(const PdServ::Session *,
        void *dest, struct timespec *) const
{
    task->pollPrepare(this, dest);

    return task->sampleTime / 2;
}

//////////////////////////////////////////////////////////////////////
void Signal::getValue( const PdServ::Session *, void *dest,
        struct timespec *t) const
{
    const PdServ::Signal *signal = this;

    task->main->poll(0, &signal, 1, dest, t);
}

//////////////////////////////////////////////////////////////////////
const void *Signal::getValue(const PdServ::SessionTaskData *std) const
{
    return static_cast<const SessionTaskData*>(std)->getValue(this);
}
