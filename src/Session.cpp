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

#include "Session.h"
#include "Main.h"
#include "Receiver.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace HRTLab;

/////////////////////////////////////////////////////////////////////////////
Session::Session(Main *m):
    main(m), receiver(new Receiver*[main->nst])
{
    main->gettime(&connectedTime);

    inBytes = 0;
    outBytes = 0;

    for (unsigned int i = 0; i < main->nst; i++)
        receiver[i] = main->newReceiver(i);
}

/////////////////////////////////////////////////////////////////////////////
Session::~Session()
{
    main->unsubscribe(this);

    for (unsigned int i = 0; i < main->nst; i++)
        delete receiver[i];

    delete[] receiver;
}

/////////////////////////////////////////////////////////////////////////////
SessionStatistics Session::getStatistics() const
{
    SessionStatistics s;

    s.remote = remoteHost;
    s.client = client;
    s.countIn = inBytes;
    s.countOut = outBytes;
    s.connectedTime = connectedTime;

    return s;
}

/////////////////////////////////////////////////////////////////////////////
void Session::resendSignalList(const Task *task) const
{
    for (unsigned int i = 0; i < main->nst; i++)
        if (receiver[i]->task == task)
            receiver[i]->resendSignalList();
}

/////////////////////////////////////////////////////////////////////////////
void Session::rxPdo()
{
    for (unsigned int i = 0; i < main->nst; i++)
        receiver[i]->process(this);
}

