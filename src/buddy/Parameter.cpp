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

#include "Main.h"
#include "../Debug.h"
#include "Parameter.h"

//////////////////////////////////////////////////////////////////////
Parameter::Parameter( const Main *main, char *parameterData,
        const SignalInfo &si):
    PdServ::Parameter(si.path(), 0x666, si.dataType(), si.ndim(), si.getDim()),
    main(main), valueBuf(parameterData), si(si), mutex(1)
{
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

    si.write(valueBuf, src, startIndex, nelem);
    int rv = main->setParameter(this, 0, this->nelem, valueBuf, &mtime);

    if (!rv)
        main->parameterChanged(this, startIndex, nelem);

    return rv;
}

//////////////////////////////////////////////////////////////////////
void Parameter::getValue(const PdServ::Session *, void* dst,
        struct timespec *time) const
{
    ost::SemaphoreLock lock(mutex);
    si.read(dst, valueBuf);
    if (time)
        *time = mtime;
}
