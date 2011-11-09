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
#include "pdserv/pdserv.h"

/////////////////////////////////////////////////////////////////////////////
struct pdserv* pdserv_create(int argc, const char *argv[], 
        const char *name, const char *version,
        int (*gettime)(struct timespec*))
{
    return reinterpret_cast<struct pdserv*>(
            new Main(argc, argv, name, version, gettime));
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
void pdserv_update(struct pdtask* task, const struct timespec *t,
        double exec_time, double cycle_time)
{
    reinterpret_cast<Task*>(task)->update(t, exec_time, cycle_time);
}

/////////////////////////////////////////////////////////////////////////////
struct variable *pdserv_signal(
        struct pdtask* pdtask,
        unsigned int decimation,
        const char *path,
        enum si_datatype_t datatype,
        const void *addr,
        size_t n,
        const unsigned int dim[]
        )
{
    Task *task = reinterpret_cast<Task*>(pdtask);

    PdServ::Variable *v =
        task->main->addSignal(task, decimation, path, datatype, addr, n, dim);

    return reinterpret_cast<struct variable *>(v);
}

/////////////////////////////////////////////////////////////////////////////
struct variable *pdserv_parameter(
        struct pdserv* pdserv,
        const char *path,
        unsigned int mode,
        enum si_datatype_t datatype,
        void *addr,
        size_t n,
        const unsigned int dim[],
        paramtrigger_t trigger = 0,
        void *priv_data = 0
        )
{
    Main *main = reinterpret_cast<Main*>(pdserv);
    Parameter *p = main->addParameter( path, mode, datatype, addr, n, dim);
    if (trigger)
        p->trigger = trigger;
    p->priv_data = priv_data;

    return
        reinterpret_cast<struct variable *>(static_cast<PdServ::Variable*>(p));
}

/////////////////////////////////////////////////////////////////////////////
void pdserv_set_alias( struct variable *var, const char *alias)
{
    reinterpret_cast<PdServ::Variable*>(var)->alias = alias;
}

/////////////////////////////////////////////////////////////////////////////
void pdserv_set_unit( struct variable *var, const char *unit)
{
    reinterpret_cast<PdServ::Variable*>(var)->unit = unit;
}

/////////////////////////////////////////////////////////////////////////////
void pdserv_set_comment( struct variable *var, const char *comment)
{
    reinterpret_cast<PdServ::Variable*>(var)->comment = comment;
}

/////////////////////////////////////////////////////////////////////////////
int pdserv_prepare(struct pdserv* pdserv)
{
    return reinterpret_cast<Main*>(pdserv)->run();
}
