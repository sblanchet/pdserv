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

#include "TimeSignal.h"
#include "Session.h"
#include "Server.h"
#include "../SessionTaskData.h"
#include "../Main.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
TimeSignal::TimeSignal(Server *s, double sampleTime):
    PdServ::Signal("/Time", sampleTime, si_double_T, 1, 0),
    server(s)
{
    t = 88;
}

/////////////////////////////////////////////////////////////////////////////
void TimeSignal::subscribe(PdServ::Session *session) const
{
    const PdServ::Signal *signal = this;
    session->newSignalList(server->main->getTask(sampleTime), &signal, 1);
}

/////////////////////////////////////////////////////////////////////////////
void TimeSignal::unsubscribe(PdServ::Session *) const
{
}

/////////////////////////////////////////////////////////////////////////////
double TimeSignal::poll(const PdServ::Session *s,
        void *buf, struct timespec *t) const
{
//    cout << __PRETTY_FUNCTION__ << endl;
    const Session *session = static_cast<const Session *>(s);
    *reinterpret_cast<double*>(buf) = *session->getDblTimePtr();

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
const void *TimeSignal::getValue(const PdServ::SessionTaskData* std) const
{
//    cout << __PRETTY_FUNCTION__ << endl;
    return static_cast<const Session*>(std->session)->getDblTimePtr();
}

/////////////////////////////////////////////////////////////////////////////
void TimeSignal::getValue(PdServ::Session *s,
        void *buf, struct timespec* t) const
{
//    cout << __PRETTY_FUNCTION__ << endl;
    poll(s, buf, t);
}
