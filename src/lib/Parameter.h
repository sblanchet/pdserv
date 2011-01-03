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

#ifndef LIB_PARAMETER
#define LIB_PARAMETER

#include <ctime>
#include <cc++/thread.h>
#include "../Parameter.h"
#include "pdcomserv/pdcomserv.h"

class Main;

class Parameter: public HRTLab::Parameter {
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

        paramtrigger_t trigger;
        void *priv_data;

        void copyValue(const char *valbuf, const struct timespec& t) const;

    private:

        mutable ost::Semaphore mutex;
        mutable struct timespec mtime;
        char *valueBuf;

        // A default function used when paramcheck or paramupdate are not
        // specified by the user
        static int copy(unsigned int tid, char checkOnly,
                void *dst, const void *src, size_t len, void *priv_data);

        // Reimplemented from HRTLab::Variable
        void getValue(char *buf, struct timespec* t = 0) const;
};

#endif //LIB_PARAMETER
