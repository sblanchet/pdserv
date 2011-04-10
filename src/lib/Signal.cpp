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
    PdServ::Signal(task, decimation, path, dtype, ndims, dim),
    addr(reinterpret_cast<const char *>(addr)),
    task(task), subscriptionIndex(dataTypeIndex[width])
{
    task->newSignal(this);
    //cout << __func__ << " addr = " << addr << endl;
}

//////////////////////////////////////////////////////////////////////
void Signal::subscribe(PdServ::Session *session) const
{
    const PdServ::Signal *s = this;
    task->subscribe(session, &s, 1);
}

//////////////////////////////////////////////////////////////////////
void Signal::unsubscribe(PdServ::Session *session) const
{
    const PdServ::Signal *s = this;
    task->unsubscribe(session, &s, 1);
}

//////////////////////////////////////////////////////////////////////
void Signal::getValue(char *buf, struct timespec* t) const
{
    const PdServ::Signal * s = this;
    task->PdServ::Task::main->getValues(&s, 1, buf, t);
}
