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

#include "Main.h"
#include "Task.h"
#include "Signal.h"
#include "Parameter.h"
#include "pdserv.h"

/////////////////////////////////////////////////////////////////////////////
struct pdserv* pdserv_create( const char *name, const char *version,
        int (*gettime)(struct timespec*))
{
    return reinterpret_cast<struct pdserv*>(
            new Main(name, version, gettime));
}

/////////////////////////////////////////////////////////////////////////////
int pdserv_config_file( struct pdserv* pdserv, const char *name)
{
    return reinterpret_cast<Main*>(pdserv)->configFile(name);
}

/////////////////////////////////////////////////////////////////////////////
struct pdtask* pdserv_create_task(struct pdserv* pdserv, double tsample,
        const char *name)
{
    return reinterpret_cast<struct pdtask*>(
            reinterpret_cast<Main*>(pdserv)->addTask(tsample, name));
}

/////////////////////////////////////////////////////////////////////////////
void pdserv_exit(struct pdserv* pdserv)
{
    delete reinterpret_cast<Main*>(pdserv);
}

/////////////////////////////////////////////////////////////////////////////
void pdserv_get_parameters(struct pdserv* pdserv, struct pdtask *task,
        const struct timespec *t)
{
    reinterpret_cast<Main*>(pdserv)->getParameters(
            reinterpret_cast<Task*>(task), t);
}

/////////////////////////////////////////////////////////////////////////////
void pdserv_update_statistics(struct pdtask* task,
        double exec_time, double cycle_time, unsigned int overrun)
{
    reinterpret_cast<Task*>(task)->updateStatistics(
            exec_time, cycle_time, overrun);
}

/////////////////////////////////////////////////////////////////////////////
void pdserv_update(struct pdtask* task, const struct timespec *t)
{
    reinterpret_cast<Task*>(task)->update(t);
}

/////////////////////////////////////////////////////////////////////////////
static PdServ::Variable::Datatype getDataType(enum pdserv_datatype_t dt)
{
    switch (dt) {
        case pd_boolean_T: return PdServ::Variable::boolean_T;
        case pd_uint8_T:   return PdServ::Variable::uint8_T;
        case pd_uint16_T:  return PdServ::Variable::uint16_T;
        case pd_uint32_T:  return PdServ::Variable::uint32_T;
        case pd_uint64_T:  return PdServ::Variable::uint64_T;
        case pd_sint8_T:   return PdServ::Variable::int8_T;
        case pd_sint16_T:  return PdServ::Variable::int16_T;
        case pd_sint32_T:  return PdServ::Variable::int32_T;
        case pd_sint64_T:  return PdServ::Variable::int64_T;
        case pd_double_T:  return PdServ::Variable::double_T;
        case pd_single_T:  return PdServ::Variable::single_T;
    }
    return PdServ::Variable::boolean_T;
}

/////////////////////////////////////////////////////////////////////////////
struct pdvariable *pdserv_signal(
        struct pdtask* pdtask,
        unsigned int decimation,
        const char *path,
        enum pdserv_datatype_t datatype,
        const void *addr,
        size_t n,
        const size_t dim[]
        )
{
    Task *task = reinterpret_cast<Task*>(pdtask);

    PdServ::Variable *v = task->main->addSignal(
            task, decimation, path, getDataType(datatype), addr, n, dim);

    return reinterpret_cast<struct pdvariable *>(v);
}

/////////////////////////////////////////////////////////////////////////////
struct pdvariable *pdserv_parameter(
        struct pdserv* pdserv,
        const char *path,
        unsigned int mode,
        enum pdserv_datatype_t datatype,
        void *addr,
        size_t n,
        const size_t dim[],
        paramtrigger_t trigger = 0,
        void *priv_data = 0
        )
{
    Main *main = reinterpret_cast<Main*>(pdserv);
    Parameter *p = main->addParameter(
            path, mode, getDataType(datatype), addr, n, dim);
    if (trigger)
        p->trigger = trigger;
    p->priv_data = priv_data;

    return reinterpret_cast<struct pdvariable *>
        (static_cast<PdServ::Variable*>(p));
}

/////////////////////////////////////////////////////////////////////////////
void pdserv_set_alias(struct pdvariable *var, const char *alias)
{
    reinterpret_cast<PdServ::Variable*>(var)->alias = alias;
}

/////////////////////////////////////////////////////////////////////////////
void pdserv_set_unit(struct pdvariable *var, const char *unit)
{
    reinterpret_cast<PdServ::Variable*>(var)->unit = unit;
}

/////////////////////////////////////////////////////////////////////////////
void pdserv_set_comment(struct pdvariable *var, const char *comment)
{
    reinterpret_cast<PdServ::Variable*>(var)->comment = comment;
}

/////////////////////////////////////////////////////////////////////////////
int pdserv_prepare(struct pdserv* pdserv)
{
    return reinterpret_cast<Main*>(pdserv)->run();
}
