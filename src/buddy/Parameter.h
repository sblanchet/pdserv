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

#ifndef BUDDY_PARAMETER
#define BUDDY_PARAMETER

#include <ctime>
#include <cc++/thread.h>
#include "../ProcessParameter.h"
#include "SignalInfo.h"

class Main;

class Parameter: public PdServ::ProcessParameter {
    public:
        Parameter ( const Main *main, char *parameterData,
                const SignalInfo& si);

        ~Parameter();

        const Main * const main;

    private:
        char * const valueBuf;      // Pointer to the real address

        const SignalInfo si;

        mutable ost::Semaphore mutex;
        mutable struct timespec mtime;

        // Reimplemented from PdServ::ProcessParameter
        int setValue(const char *buf,
                size_t startIndex, size_t nelem) const;

        // Reimplemented from PdServ::Variable
        void getValue(const PdServ::Session *, void *buf,
                struct timespec* t = 0) const;
};

#endif //BUDDY_PARAMETER
