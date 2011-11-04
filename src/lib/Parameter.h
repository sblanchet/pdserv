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

#ifndef LIB_PARAMETER
#define LIB_PARAMETER

#include <ctime>
#include <cc++/thread.h>
#include "../Parameter.h"
#include "pdserv/pdserv.h"

class Main;

class Parameter: public PdServ::Parameter {
    public:
        Parameter ( Main *main,
                const char *path,
                unsigned int mode,
                enum si_datatype_t dtype,
                void *addr,
                unsigned int ndims = 1,
                const unsigned int *dim = 0);

        ~Parameter();

        char * const addr;      // Pointer to the real address
        const Main * const main;

        paramtrigger_t trigger;
        void *priv_data;

    private:
        mutable ost::Semaphore mutex;
        mutable struct timespec mtime;
        char *valueBuf;

        // Reimplemented from PdServ::Parameter
        int setValue(const char *buf,
                size_t startIndex, size_t nelem) const;

        // Reimplemented from PdServ::Variable
        void getValue(PdServ::Session *,
                char *buf, struct timespec* t = 0) const;

        // A default function used when paramcheck or paramupdate are not
        // specified by the user
        static int copy(struct pdtask *task, const struct variable *parameter,
                void *dst, const void *src, size_t len, void *priv_data);
};

#endif //LIB_PARAMETER
