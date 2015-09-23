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

#include "ProcessParameter.h"
#include "Main.h"

using namespace PdServ;

//////////////////////////////////////////////////////////////////////
ProcessParameter::ProcessParameter(
        Main* main,
        char* const* addr,
        const struct timespec* mtime,
        const std::string& path,
        unsigned int mode,
        const PdServ::DataType& dtype,
        size_t ndims,
        const size_t *dim):
    Parameter(path, mode, dtype, ndims, dim),
    main(main), valueBuf(addr), mtime(mtime)
{
}

//////////////////////////////////////////////////////////////////////
int ProcessParameter::setValue(const PdServ::Session* session,
        const char *buf, size_t offset, size_t count) const
{
    return main->setValue(this, session, buf, offset, count);
}

//////////////////////////////////////////////////////////////////////
void ProcessParameter::getValue(const PdServ::Session* /*session*/,
        void* buf,  struct timespec *time) const
{
    ost::ReadLock lock(mutex);

    std::copy(*valueBuf, *valueBuf + memSize, reinterpret_cast<char*>(buf));
    if (time)
        *time = *mtime;
}

//////////////////////////////////////////////////////////////////////
int ProcessParameter::setValue(
        const char *buf, size_t offset, size_t count) const
{
    ost::WriteLock lock(mutex);

    char *dst = *valueBuf + offset;

    // Backup data in case of failure
    char backup[count];
    std::copy(dst, dst + count, backup);

    // Copy data into shared memory
    std::copy(buf, buf + count, dst);

    int rv = main->setParameter(this, offset, count);

    if (rv)
        // There was a error, restore backup
        std::copy(backup, backup + count, dst);

    return rv;
}

//////////////////////////////////////////////////////////////////////
void ProcessParameter::print(
        std::ostream& os, size_t offset, size_t count) const
{
    dtype.print(os,
            *valueBuf, *valueBuf + offset, *valueBuf + offset + count);
}
