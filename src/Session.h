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

#ifndef SESSION_H
#define SESSION_H

#include <string>
#include <list>
#include <ctime>

namespace PdServ {

class Task;
class Signal;
class Main;
class SessionTaskData;
class SessionShadow;
class SessionStatistics;
class TaskStatistics;

class Session {
    public:
        Session(const Main *main);
        virtual ~Session();

        const TaskStatistics *getTaskStatistics(const Task *task) const;
        const struct timespec *getTaskTime(const Task *) const;

        // These methods are called from within the context of rxPdo
        virtual void newSignal(const Task *, const Signal *) = 0;
        virtual void newSignalData(const SessionTaskData*,
                const struct timespec *) = 0;

        const Main * const main;

    protected:
        SessionShadow * const shadow;

        bool rxPdo();

        struct timespec connectedTime;
    private:
};

}
#endif //SESSION_H
