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

#include <algorithm>
#include <cerrno>

#include "Parameter.h"
#include "Main.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

//////////////////////////////////////////////////////////////////////
Parameter::Parameter(
        Main *main,
        const char *path,
        unsigned int mode,
        enum si_datatype_t dtype,
        void *addr,
        unsigned int ndims,
        const unsigned int *dim):
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
int Parameter::setValue(const char* src,
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
void Parameter::getValue(PdServ::Session *,
        void* buf, struct timespec *time) const
{
    ost::SemaphoreLock lock(mutex);
    std::copy(valueBuf, valueBuf + memSize, reinterpret_cast<char*>(buf));
    if (time)
        *time = mtime;
}

//////////////////////////////////////////////////////////////////////
int Parameter::copy(struct pdtask *, const struct variable *,
        void *buf, const void *src, size_t len, void *)
{
//    cout << __PRETTY_FUNCTION__ << checkOnly << endl;
    std::copy( reinterpret_cast<const char*>(src), 
            reinterpret_cast<const char*>(src)+len,
            reinterpret_cast<char*>(buf));

    return 0;
}
