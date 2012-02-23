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

#ifndef BUDDY_SESSIONSHADOW_H
#define BUDDY_SESSIONSHADOW_H

#include "../SessionShadow.h"
#include "SessionTaskData.h"

namespace PdServ {
    class Task;
    class Session;
}

class Task;

class SessionShadow: public PdServ::SessionShadow {
    public:
        SessionShadow(PdServ::Session *session,
                const Task *task, unsigned int current,
                const unsigned int* photoReady, const char *album,
                const struct app_properties *app_properties);

        ~SessionShadow();

    private:
        SessionTaskData taskData;

        unsigned int current;
        const unsigned int * const photoReady;
        unsigned int lastPhoto;
        size_t count;

        // Reimplemented from PdServ::SessionShadow
        bool rxPdo();
        const PdServ::TaskStatistics *getTaskStatistics(
                const PdServ::Task *) const;
        const struct timespec *getTaskTime(const PdServ::Task *) const;
};

#endif //BUDDY_SESSIONSHADOW_H
