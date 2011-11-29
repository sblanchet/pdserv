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

#include "Signal.h"

#include "../Session.h"
#include "../Debug.h"
#include "Main.h"
//#include "SessionTaskData.h"

//////////////////////////////////////////////////////////////////////
Signal::Signal( const Main *main, double sampleTime, const SignalInfo& si):
    PdServ::Signal(si.path(), sampleTime, si.dataType(), si.ndim(), si.dim()),
    main(main), offset(si.si->offset), info(si)
{
}

//////////////////////////////////////////////////////////////////////
void Signal::subscribe(PdServ::Session *session) const
{
    const PdServ::Signal *signal = this;
    session->newSignalList(0, &signal, 1);
}

//////////////////////////////////////////////////////////////////////
void Signal::unsubscribe(PdServ::Session *s) const
{
}

//////////////////////////////////////////////////////////////////////
double Signal::poll(const PdServ::Session *,
        void *dest, struct timespec *) const
{
    return 0;
}

//////////////////////////////////////////////////////////////////////
void Signal::getValue( const PdServ::Session *, void *dest,
        size_t start, size_t count, struct timespec *t) const
{
    const PdServ::Signal *signal = this;
    char buf[memSize];

    main->poll(0, &signal, 1, buf, t);
    std::copy(buf + start * width, buf + (start + count) * width,
            reinterpret_cast<char*>(dest));
}

//////////////////////////////////////////////////////////////////////
const void *Signal::getValue(const PdServ::SessionTaskData *std) const
{
//    return static_cast<const SessionTaskData*>(std)->getValue(this);
    return 0;
}
