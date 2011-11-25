/*****************************************************************************
 *
 *  $Id: SessionShadow.h,v 26930f4ff1e7 2011/11/18 17:44:41 lerichi $
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

#ifndef BUDDY_SESSIONSHADOW_H
#define BUDDY_SESSIONSHADOW_H

#include "../SessionShadow.h"

#include <queue>
#include <cc++/thread.h>

namespace PdServ {
    class Task;
    class Session;
    class SessionStatistics;
}

class Main;
class SessionTaskData;

class SessionShadow: public PdServ::SessionShadow {
    public:
        SessionShadow(/*const Main *main,*/ PdServ::Session *session);

        ~SessionShadow();

    private:
//        const Main * const main;
        PdServ::Session * const session;

        mutable ost::Semaphore mutex;

        std::queue<size_t> photos;

        // Reimplemented from PdServ::SessionShadow
        bool rxPdo();
        const PdServ::TaskStatistics *getTaskStatistics(
                const PdServ::Task *) const;
};

#endif //BUDDY_SESSIONSHADOW_H
