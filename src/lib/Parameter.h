/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdserv library.
 *
 *  The pdserv library is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or (at
 *  your option) any later version.
 *
 *  The pdserv library is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with the pdserv library. If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#ifndef LIB_PARAMETER
#define LIB_PARAMETER

#include <ctime>
#include <cc++/thread.h>
#include "../Parameter.h"
#include "pdserv.h"

class Main;

class Parameter: public PdServ::Parameter {
    public:
        Parameter ( Main const* main,
                const char *path,
                unsigned int mode,
                const PdServ::DataType& dtype,
                void *addr,
                size_t ndims = 1,
                const size_t *dim = 0);

        char * const addr;      // Pointer to the real address

        paramtrigger_t trigger;
        void *priv_data;

        mutable char *valueBuf;

    private:
        const Main * const main;
        mutable ost::ThreadLock mutex;
        mutable struct timespec mtime;

        // Reimplemented from PdServ::Parameter
        int setValue(const PdServ::Session* session,
                const char *buf, size_t offset, size_t count) const;

        // Reimplemented from PdServ::Variable
        void getValue(const PdServ::Session* session,
                void *buf, struct timespec* t = 0) const;

        // A default function used when paramcheck or paramupdate are not
        // specified by the user
        static int copy(struct pdtask *task,
                const struct pdvariable *parameter, void *dst,
                const void *src, size_t len, void *priv_data);
};

#endif //LIB_PARAMETER
