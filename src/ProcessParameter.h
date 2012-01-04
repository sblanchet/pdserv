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


#ifndef PROCESS_PARAMETER_H
#define PROCESS_PARAMETER_H

#include "Parameter.h"

namespace PdServ {

class Session;
class Main;

class ProcessParameter: public Parameter {
    public:
        ProcessParameter ( Main const* main,
                std::string const& path,
                unsigned int mode,
                Datatype dtype,
                size_t ndims = 1,
                const size_t *dim = 0);

        virtual int setValue(const char *buf,
                size_t startIndex, size_t nelem) const = 0;

    private:
        Main const * const main;

        // Reimplemented from PdServ::Parameter
        int setValue(const Session *, const char *buf,
                size_t startIndex, size_t nelem) const;
};

}

#endif //PROCESS_PARAMETER_H