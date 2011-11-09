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
SessionTaskData::SessionTaskData(Task* t, PdServ::Session* s,
        struct Pdo *pdo, size_t signalCount,
        unsigned int signalListId, const Signal * const *sp, size_t nelem):
    PdServ::SessionTaskData(t), task(t), session(s), 
    signalPosition(signalCount), pdo(pdo)
{
    this->signalListId = signalListId;
    loadSignalList(sp, nelem);
}

////////////////////////////////////////////////////////////////////////////
void SessionTaskData::rxPdo()
{
    task->rxPdo(&pdo, this);
}

////////////////////////////////////////////////////////////////////////////
void SessionTaskData::loadSignalList(const Signal * const *sp, size_t n)
{
    std::fill(signalPosition.begin(),  signalPosition.end(), ~0U);
    for (size_t i = 0; i < n; ++i)
        signalPosition[sp[i]->index] = i;
}

////////////////////////////////////////////////////////////////////////////
void SessionTaskData::newSignalList(
        unsigned int signalListId, const Signal * const *sp, size_t n)
{
    loadSignalList(sp, n);
    const PdServ::Signal *signals[n];

    std::copy(sp, sp+n, signals);
    session->newSignalList(task, signals, n);
}

////////////////////////////////////////////////////////////////////////////
void SessionTaskData::newSignalData( unsigned int id, 
        const PdServ::TaskStatistics *stats, const char *buf)
{
    signalBuffer = buf;
    signalListId = id;
    taskStatistics = stats;

    session->newSignalData(this);
}

////////////////////////////////////////////////////////////////////////////
const char *SessionTaskData::getValue(const PdServ::Signal *s) const
{
    const Signal *signal = static_cast<const Signal*>(s);

    size_t n = signalPosition[signal->index];

    return n != ~0U ? signalBuffer + n : 0;
}
