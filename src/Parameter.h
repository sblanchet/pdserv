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


#ifndef PARAMETER
#define PARAMETER

#include "Variable.h"

struct timespec;

namespace HRTLab {

class Main;

class Parameter: public Variable {
    public:
        Parameter ( Main *main,
                const char *path,
                unsigned int mode,
                enum si_datatype_t dtype,
                unsigned int ndims = 1,
                const size_t *dim = 0);

        virtual ~Parameter();

        const Main * const main;

        const unsigned int mode;

        int setValue(const char *buf) const;

    protected:
};

}

#endif //PARAMETER
