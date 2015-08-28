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
#include "Parameter.h"
#include "Main.h"

//////////////////////////////////////////////////////////////////////
Parameter::Parameter( const Main *main, char *parameterData,
        const SignalInfo &si):
    PdServ::Parameter(
            si.path(), 0x666, si.dataType(), si.ndim(), si.getDim()),
    main(main), valueBuf(parameterData), si(si)
{
    mtime.tv_sec = 0;
    mtime.tv_nsec = 0;
}

//////////////////////////////////////////////////////////////////////
int Parameter::setValue(const PdServ::Session* session,
        const char* src, size_t offset, size_t count) const
{
    char copy[memSize];
    {
        ost::WriteLock lock(mutex);

        // Make a backup of the data to be changed
        std::copy(valueBuf + offset, valueBuf + offset + count, copy);

        // Copy in new data to valueBuf
        std::copy(src, src + count, valueBuf + offset);

        int rv = main->setParameter(valueBuf + offset, count, &mtime);
        if (rv) {
            // Restore valueBuf and return
            std::copy(copy, copy + count, valueBuf + offset);
            return rv;
        }

        // Copy new data
        std::copy(valueBuf, valueBuf + memSize, copy);
    }

    // Tell main that the value has changed, with a copy of the value
    main->parameterChanged(session, this, copy, offset, count);

    return 0;
}

//////////////////////////////////////////////////////////////////////
void Parameter::getValue(const PdServ::Session *, void* dst,
        struct timespec *time) const
{
    ost::ReadLock lock(mutex);
    std::copy(valueBuf, valueBuf + memSize, reinterpret_cast<char*>(dst));
    if (time)
        *time = mtime;
}
