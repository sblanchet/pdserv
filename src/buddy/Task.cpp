/*****************************************************************************
 *
 *  $Id: Signal.cpp,v ca2d0581b018 2011/11/18 21:54:07 lerichi $
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

#include "Task.h"
#include "Signal.h"

//////////////////////////////////////////////////////////////////////
Task::Task( Main *main, double sampleTime):
    PdServ::Task(sampleTime), main(main)
{
}

//////////////////////////////////////////////////////////////////////
const PdServ::Signal *Task::addSignal(const SignalInfo& si)
{
    Signal *s = new Signal(main, sampleTime, si);
    signals.push_back(s);
    return s;
}
