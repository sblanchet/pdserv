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

#include <cerrno>

#include "ProcessParameter.h"
#include "Main.h"

using namespace PdServ;

//////////////////////////////////////////////////////////////////////
ProcessParameter::ProcessParameter (Main const* main,
        const std::string& path, unsigned int mode,
        const DataType& dtype, size_t ndims, const size_t *dim):
    Parameter(path, mode, dtype, ndims, dim),
    main(main)
{
}

//////////////////////////////////////////////////////////////////////
ProcessParameter::~ProcessParameter ()
{
}

//////////////////////////////////////////////////////////////////////
int ProcessParameter::setValue(const PdServ::Session *session, const char* src,
                size_t offset, size_t count) const
{
    return main->setParameter(session, this, offset, count, src);
}
