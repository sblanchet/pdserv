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

#include <algorithm>
#include <cerrno>

#include "Parameter.h"
#include "Main.h"

//////////////////////////////////////////////////////////////////////
Parameter::Parameter(
        Main const* main,
        const char *path,
        unsigned int mode,
        Datatype dtype,
        void *addr,
        size_t ndims,
        const size_t *dim):
    PdServ::ProcessParameter(main, path, mode, dtype, ndims, dim),
    addr(reinterpret_cast<char*>(addr)), main(main), mutex(1)
{
    trigger = copy;

    mtime.tv_sec = 0;
    mtime.tv_nsec = 0;
}

//////////////////////////////////////////////////////////////////////
Parameter::~Parameter()
{
}

//////////////////////////////////////////////////////////////////////
int Parameter::setValue(const char* src, size_t startIndex, size_t nelem) const
{
    ost::SemaphoreLock lock(mutex);

    return main->setParameter(this, startIndex, nelem, src, &mtime);
}

//////////////////////////////////////////////////////////////////////
void Parameter::getValue(const PdServ::Session *,
        void* valueBuf,  struct timespec *time) const
{
    ost::SemaphoreLock lock(mutex);

    std::copy(this->valueBuf, this->valueBuf + memSize,
            reinterpret_cast<char*>(valueBuf));
    if (time)
        *time = mtime;
}

//////////////////////////////////////////////////////////////////////
int Parameter::copy(struct pdtask *, const struct pdvariable *,
        void *buf, const void *src, size_t len, void *)
{
//    cout << __PRETTY_FUNCTION__ << checkOnly << endl;
    std::copy( reinterpret_cast<const char*>(src), 
            reinterpret_cast<const char*>(src)+len,
            reinterpret_cast<char*>(buf));

    return 0;
}
