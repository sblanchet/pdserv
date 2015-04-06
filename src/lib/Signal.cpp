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

#include "../Debug.h"

#include "../SessionTask.h"
#include "Signal.h"
#include "Main.h"
#include "SessionTaskData.h"
#include "../DataType.h"

//////////////////////////////////////////////////////////////////////
const size_t Signal::dataTypeIndex[PdServ::DataType::maxWidth+1] = {
    3 /*0*/, 3 /*1*/, 2 /*2*/, 3 /*3*/,
    1 /*4*/, 3 /*5*/, 3 /*6*/, 3 /*7*/, 0 /*8*/
};

//////////////////////////////////////////////////////////////////////
Signal::Signal( Task *task,
        size_t index,
        unsigned int decimation,
        const char *path,
        const PdServ::DataType& dtype,
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
void Signal::subscribe(PdServ::SessionTask *s) const
{
    ost::SemaphoreLock lock(mutex);

    if (sessions.empty()) {
//        log_debug("Request signal from RT task %p", s);
        task->subscribe(this, true);
    }
    else if (s->sessionTaskData->isBusy(this)) {
//        log_debug("Signal already transferred");
        s->newSignal(this);
    }

    sessions.insert(s);
}

//////////////////////////////////////////////////////////////////////
void Signal::unsubscribe(PdServ::SessionTask *s) const
{
    ost::SemaphoreLock lock(mutex);

    if (sessions.erase(s) and sessions.empty())
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
void Signal::getValue( const PdServ::Session* session, void *dest,
        struct timespec *t) const
{
    const PdServ::Signal *signal = this;

    task->main->poll(session, &signal, 1, dest, t);
}

//////////////////////////////////////////////////////////////////////
const char *Signal::getValue(const PdServ::SessionTask* st) const
{
    return st->sessionTaskData->getValue(this);
}
