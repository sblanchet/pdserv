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

#ifndef SIGNAL_H
#define SIGNAL_H

#include "Variable.h"

namespace PdServ {

class Session;
class SessionTaskData;

class Signal: public Variable {
    public:
        Signal( const char *path,
                double sampleTime,
                enum si_datatype_t dtype,
                unsigned int ndims = 1,
                const unsigned int *dim = 0);

        virtual ~Signal();

        const double sampleTime;

        virtual void subscribe(Session *) const = 0;
        virtual void unsubscribe(Session *) const = 0;

        virtual double poll(const Session *s,
                void *buf, struct timespec *t) const = 0;

        virtual const void *getValue(const SessionTaskData*) const = 0;

        // Reimplemented from PdServ::Variable
        virtual void getValue(Session*,
                void *, struct timespec * = 0) const = 0;
    private:

};

}

#endif //SIGNAL_H
