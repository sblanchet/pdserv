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

#include <algorithm>
#include <cerrno>

#include "Parameter.h"
#include "Main.h"

//////////////////////////////////////////////////////////////////////
Parameter::Parameter(
        Main *main,
        const char *path,
        unsigned int mode,
        Datatype dtype,
        void *addr,
        size_t ndims,
        const size_t *dim):
    PdServ::Parameter(path, mode, dtype, ndims, dim),
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
int Parameter::setValue(const PdServ::Session *, const char* src,
                size_t startIndex, size_t nelem) const
{
    if (startIndex + nelem > this->nelem)
        return -EINVAL;

    ost::SemaphoreLock lock(mutex);

    int rv = main->setParameter(this, startIndex, nelem, src, &mtime);

    if (!rv)
        main->parameterChanged(this, startIndex, nelem);

    return rv;
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
