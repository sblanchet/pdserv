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

#include "Signal.h"

extern "C" {
#include <etl_data_info.h>
}

//#include "Task.h"
//#include "Main.h"
//#include "SessionTaskData.h"

static PdServ::Variable::Datatype dataType(const enum si_datatype_t& dt)
{
    switch (dt) {
        case si_boolean_T: return PdServ::Variable::boolean_T;
        case si_uint8_T:   return PdServ::Variable::uint8_T;
        case si_sint8_T:   return PdServ::Variable::int8_T;
        case si_uint16_T:  return PdServ::Variable::uint16_T;
        case si_sint16_T:  return PdServ::Variable::int16_T;
        case si_uint32_T:  return PdServ::Variable::uint32_T;
        case si_sint32_T:  return PdServ::Variable::int32_T;
        case si_double_T:  return PdServ::Variable::double_T;
        case si_single_T:  return PdServ::Variable::single_T;
        default:           return PdServ::Variable::double_T;
    }

}

// //////////////////////////////////////////////////////////////////////
// const size_t Signal::dataTypeIndex[PdServ::Variable::maxWidth+1] = {
//     3 /*0*/, 3 /*1*/, 2 /*2*/, 3 /*3*/,
//     1 /*4*/, 3 /*5*/, 3 /*6*/, 3 /*7*/, 0 /*8*/
// };

//////////////////////////////////////////////////////////////////////
Signal::Signal( //Task *task,
        const struct signal_info *si, double sampleTime):
    PdServ::Signal(makePath(si), sampleTime,
            dataType(si->data_type), 1 + (si->dim[1] > 0))
//    addr(reinterpret_cast<const char *>(addr)),
//    index(index), task(task),
//    mutex(1)
{
}

//////////////////////////////////////////////////////////////////////
std::string Signal::makePath(const struct signal_info *si)
{
    return std::string(1,'/').append(si->path).append(1,'/').append(si->name);
}

//////////////////////////////////////////////////////////////////////
void Signal::subscribe(PdServ::Session *s) const
{
//    ost::SemaphoreLock lock(mutex);
//
//    if (sessions.empty())
//        task->subscribe(this, true);
//
//    sessions.insert(s);
}

//////////////////////////////////////////////////////////////////////
void Signal::unsubscribe(PdServ::Session *s) const
{
//    ost::SemaphoreLock lock(mutex);
//
//    sessions.erase(s);
//
//    if (sessions.empty())
//        task->subscribe(this, false);
}

//////////////////////////////////////////////////////////////////////
double Signal::poll(const PdServ::Session *,
        void *dest, struct timespec *) const
{
//    task->pollPrepare(this, dest);
//
//    return task->sampleTime / 2;
    return 0;
}

//////////////////////////////////////////////////////////////////////
void Signal::getValue( const PdServ::Session *, void *buf,
        size_t start, size_t count, struct timespec *t) const
{
//    const PdServ::Signal *s = this;
//    task->main->poll(0, &s, 1, buf, t);
}

//////////////////////////////////////////////////////////////////////
const void *Signal::getValue(const PdServ::SessionTaskData *std) const
{
//    return static_cast<const SessionTaskData*>(std)->getValue(this);
    return 0;
}
