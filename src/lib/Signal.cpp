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

#include "Signal.h"
#include "Task.h"
#include "Main.h"
#include "../Main.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

//////////////////////////////////////////////////////////////////////
const size_t Signal::dataTypeIndex[PdServ::Variable::maxWidth+1] = {
    3 /*0*/, 3 /*1*/, 2 /*2*/, 3 /*3*/,
    1 /*4*/, 3 /*5*/, 3 /*6*/, 3 /*7*/, 0 /*8*/
};

//////////////////////////////////////////////////////////////////////
Signal::Signal( Task *task,
        unsigned int decimation,
        const char *path,
        enum si_datatype_t dtype,
        const void *addr,
        unsigned int ndims,
        const unsigned int *dim):
    PdServ::Signal( task->main, path,
            task->sampleTime * decimation, dtype, ndims, dim),
    addr(reinterpret_cast<const char *>(addr)),
    task(task), subscriptionIndex(dataTypeIndex[width]),
    mutex(1)
{
    task->newSignal(this);
}

//////////////////////////////////////////////////////////////////////
void Signal::subscribe(PdServ::Session *s) const
{
    ost::SemaphoreLock lock(mutex);

    if (sessions.empty())
        task->subscribe(this, true);

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
void Signal::poll(const PdServ::Session *, char *buf) const
{
    pollDest = buf;
    task->pollPrepare(this);
}

//////////////////////////////////////////////////////////////////////
void Signal::getValue(char *buf, struct timespec *) const
{
    const PdServ::Signal *s = this;
    task->main->poll(0, &s, 1, buf);
}
