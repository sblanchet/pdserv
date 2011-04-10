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
#include "Signal.h"
#include "Parameter.h"
#include "pdserv/pdserv.h"

/////////////////////////////////////////////////////////////////////////////
struct pdserv* pdserv_create(int argc, const char *argv[], 
        const char *name, const char *version, double baserate,
        unsigned int nst, const unsigned int decimation[],
        int (*gettime)(struct timespec*))
{
    return reinterpret_cast<struct pdserv*>(
            new Main(argc, argv, name, version, baserate, nst,
                decimation, gettime));
}

/////////////////////////////////////////////////////////////////////////////
void pdserv_exit(struct pdserv* pdserv)
{
    delete reinterpret_cast<Main*>(pdserv);
}

/////////////////////////////////////////////////////////////////////////////
void pdserv_update(struct pdserv* pdserv, int st, const struct timespec *t)
{
    reinterpret_cast<Main*>(pdserv)->update(st, t);
}

/////////////////////////////////////////////////////////////////////////////
struct variable *pdserv_signal(
        struct pdserv* pdserv,
        unsigned int tid,
        unsigned int decimation,
        const char *path,
        enum si_datatype_t datatype,
        const void *addr,
        size_t n,
        const unsigned int dim[]
        )
{
    Main *main = reinterpret_cast<Main*>(pdserv);

    if (tid >= main->nst)
        return 0;

    PdServ::Variable *v = new Signal(main->task[tid],
            decimation, path, datatype, addr, n, dim);

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
        paramtrigger_t _trigger = 0,
        void *_priv_data = 0
        )
{
    Parameter *p = new Parameter(reinterpret_cast<Main*>(pdserv),
            path, mode, datatype, addr, n, dim);
    p->trigger = _trigger;
    p->priv_data = _priv_data;

    return
        reinterpret_cast<struct variable *>(static_cast<PdServ::Variable*>(p));
}

/////////////////////////////////////////////////////////////////////////////
void pdserv_set_alias( struct variable *var, const char *_alias)
{
    reinterpret_cast<PdServ::Variable*>(var)->alias = _alias;
}

/////////////////////////////////////////////////////////////////////////////
void pdserv_set_unit( struct variable *var, const char *_unit)
{
    reinterpret_cast<PdServ::Variable*>(var)->unit = _unit;
}

/////////////////////////////////////////////////////////////////////////////
void pdserv_set_comment( struct variable *var, const char *_comment)
{
    reinterpret_cast<PdServ::Variable*>(var)->comment = _comment;
}

/////////////////////////////////////////////////////////////////////////////
int pdserv_init(struct pdserv* pdserv)
{
    return reinterpret_cast<Main*>(pdserv)->init();
}
