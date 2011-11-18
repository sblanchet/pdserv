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

#include "Session.h"
#include "SessionShadow.h"
#include "SessionStatistics.h"
#include "Main.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace PdServ;

/////////////////////////////////////////////////////////////////////////////
Session::Session(const Main *m): main(m), shadow(main->newSession(this))
{
    main->gettime(&connectedTime);

    inBytes = 0;
    outBytes = 0;
}

/////////////////////////////////////////////////////////////////////////////
Session::~Session()
{
    delete shadow;
}

/////////////////////////////////////////////////////////////////////////////
void Session::getSessionStatistics(std::list<SessionStatistics> &stats) const
{
    SessionStatistics s;

    s.remote = remoteHost;
    s.client = client;
    s.countIn = inBytes;
    s.countOut = outBytes;
    s.connectedTime = connectedTime;

    stats.push_back(s);
}

/////////////////////////////////////////////////////////////////////////////
bool Session::rxPdo()
{
    return shadow->rxPdo();
}

/////////////////////////////////////////////////////////////////////////////
void Session::resendSignalList(const Task *task) const
{
//    for (unsigned int i = 0; i < main->getTasks().size(); i++)
//        if (receiver[i]->task == task)
//            receiver[i]->resendSignalList();
}
