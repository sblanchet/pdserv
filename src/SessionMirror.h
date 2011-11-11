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

#ifndef SESSIONMIRROR_H
#define SESSIONMIRROR_H

#include <set>

namespace PdServ {

class Session;
class Task;
class TaskStatistics;

class SessionMirror {
    public:
        virtual ~SessionMirror() {}

        virtual bool rxPdo() = 0;
        virtual const TaskStatistics *getTaskStatistics(const Task *) const = 0;

    protected:

    private:
};

}
#endif //SESSIONMIRROR_H
