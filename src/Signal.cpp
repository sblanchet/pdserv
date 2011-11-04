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
#include "Main.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace PdServ;

//////////////////////////////////////////////////////////////////////
Signal::Signal( Main *main,
        const char *path,
        double ts,
        enum si_datatype_t dtype,
        unsigned int ndims,
        const unsigned int *dim):
    Variable(path, dtype, ndims, dim),
    sampleTime(ts)
{
    main->newSignal(this);
}

//////////////////////////////////////////////////////////////////////
Signal::~Signal()
{
}
