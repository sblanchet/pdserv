/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdcomserv package.
 *
 *  pdcomserv is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pdcomserv is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pdcomserv. See COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include "config.h"

#include <algorithm>

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
    HRTLab::Parameter(main, path, mode, dtype, ndims, dim),
    addr(reinterpret_cast<char*>(addr)), mutex(1)
{
    trigger = copy;

    mtime.tv_sec = 0;
    mtime.tv_nsec = 0;

    valueBuf = new char[memSize];
}

//////////////////////////////////////////////////////////////////////
Parameter::~Parameter()
{
    delete[] valueBuf;
}

//////////////////////////////////////////////////////////////////////
void Parameter::copyValue(const char* src, const struct timespec& time) const
{
    ost::SemaphoreLock lock(mutex);

    std::copy(src, src + memSize, valueBuf);
    mtime = time;
}

//////////////////////////////////////////////////////////////////////
void Parameter::getValue(char* dst, struct timespec *time) const
{
    ost::SemaphoreLock lock(mutex);
    std::copy(valueBuf, valueBuf + memSize, dst);
    if (time)
        *time = mtime;
}

//////////////////////////////////////////////////////////////////////
int Parameter::copy(unsigned int tid, char checkOnly,
        void *dst, const void *src, size_t len, void *)
{
//    cout << __PRETTY_FUNCTION__ << checkOnly << endl;
    if (checkOnly)
        return 0;

    std::copy( reinterpret_cast<const char*>(src), 
            reinterpret_cast<const char*>(src)+len,
            reinterpret_cast<char*>(dst));

    return 0;
}
