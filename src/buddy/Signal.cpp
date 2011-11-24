/*****************************************************************************
 *
 *  $Id: Signal.cpp,v ca2d0581b018 2011/11/18 21:54:07 lerichi $
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

//#include "Task.h"
//#include "Main.h"
//#include "SessionTaskData.h"

//////////////////////////////////////////////////////////////////////
Signal::Signal( //Task *task,
                double sampleTime, const SignalInfo& si):
    PdServ::Signal(si.path(), sampleTime, si.dataType(), si.ndim(), si.dim()),
    si(si)
//    addr(reinterpret_cast<const char *>(addr)),
//    index(index), task(task),
//    mutex(1)
{
}

//////////////////////////////////////////////////////////////////////
void Signal::subscribe(PdServ::Session *s) const
{
//    ost::SemaphoreLock lock(mutex);
//
//    if (sessions.empty())
//        task->subscribe(this, true);
//
//    sessions.insert(s);
}

//////////////////////////////////////////////////////////////////////
void Signal::unsubscribe(PdServ::Session *s) const
{
//    ost::SemaphoreLock lock(mutex);
//
//    sessions.erase(s);
//
//    if (sessions.empty())
//        task->subscribe(this, false);
}

//////////////////////////////////////////////////////////////////////
double Signal::poll(const PdServ::Session *,
        void *dest, struct timespec *) const
{
//    task->pollPrepare(this, dest);
//
//    return task->sampleTime / 2;
    return 0;
}

//////////////////////////////////////////////////////////////////////
void Signal::getValue( const PdServ::Session *, void *buf,
        size_t start, size_t count, struct timespec *t) const
{
//    const PdServ::Signal *s = this;
//    task->main->poll(0, &s, 1, buf, t);
}

//////////////////////////////////////////////////////////////////////
const void *Signal::getValue(const PdServ::SessionTaskData *std) const
{
//    return static_cast<const SessionTaskData*>(std)->getValue(this);
    return 0;
}
