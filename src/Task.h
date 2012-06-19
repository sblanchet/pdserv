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

#ifndef TASK_H
#define TASK_H

#include <list>

namespace PdServ {

class Signal;
class SessionTask;
class TaskStatistics;

class Task {
    public:
        Task(double sampleTime);
        ~Task();

        const double sampleTime;

        typedef std::list<const Signal*> Signals;
        const Signals& getSignals() const;

        virtual void prepare(SessionTask *) const = 0;
        virtual void cleanup(const SessionTask *) const = 0;
        virtual bool rxPdo(SessionTask *, const struct timespec **tasktime,
                const PdServ::TaskStatistics **taskStatistics) const = 0;

    protected:
        Signals signals;

    private:
};

}
#endif // TASK_H
