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
#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

#include "../Session.h"
#include "SessionTaskData.h"
#include "Task.h"
#include "Signal.h"

////////////////////////////////////////////////////////////////////////////
SessionTaskData::SessionTaskData( PdServ::Session* s, Task* t):
    PdServ::SessionTaskData(s, t), task(t), session(s)
{
    pdoError = false;
    pdoSize = 0;

    signalPosition.resize(task->signalCount());
    const Signal *signals[signalPosition.size()];
    size_t nelem;

    task->getSignalList(signals, &nelem, &signalListId);
    loadSignalList(signals, nelem);

    task->initSession(signalListId, &pdo, &taskStatistics);
}

////////////////////////////////////////////////////////////////////////////
bool SessionTaskData::rxPdo()
{
    return task->rxPdo(&pdo, this) or pdoError;
}

////////////////////////////////////////////////////////////////////////////
void SessionTaskData::loadSignalList(const Signal * const *sp, size_t n)
{
    std::fill(signalPosition.begin(),  signalPosition.end(), ~0U);
    size_t pos = 0;
    for (size_t i = 0; i < n; ++i) {
        signalPosition[sp[i]->index] = pos;
        pos += sp[i]->memSize;
    }
}

////////////////////////////////////////////////////////////////////////////
void SessionTaskData::newSignalList(
        unsigned int signalListId, const Signal * const *sp, size_t n)
{
    loadSignalList(sp, n);
    const PdServ::Signal *signals[n];

    this->signalListId = signalListId;
    std::copy(sp, sp+n, signals);

    pdoSize = 0;
    for (size_t i = 0; i < n; i++)
        pdoSize += sp[i]->memSize;

    session->newSignalList(task, signals, n);
}

////////////////////////////////////////////////////////////////////////////
void SessionTaskData::newSignalData( unsigned int id, 
        const PdServ::TaskStatistics *stats,
        const char *buf, unsigned int buflen)
{
//    cout << __func__ << signalListId << "==" << id << ' '
//        << pdoSize << "==" << buflen;
    pdoError |= !(signalListId == id and pdoSize == buflen);
//    cout << ' ' << pdoError << endl;

    signalBuffer = buf;
    taskStatistics = stats;

    if (!pdoError)
        session->newSignalData(this);
}

////////////////////////////////////////////////////////////////////////////
const char *SessionTaskData::getValue(const PdServ::Signal *s) const
{
    const Signal *signal = static_cast<const Signal*>(s);

    size_t n = signalPosition[signal->index];

//    cout << pdoError << ' ' << pdoSize << ' ' << n + signal->memSize << endl;
//    return 0;
    return (pdoError or n + signal->memSize > pdoSize)
        ? 0 : signalBuffer + n;
}
