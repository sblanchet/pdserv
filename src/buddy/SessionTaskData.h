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

#ifndef BUDDY_SESSIONTASKDATA_H
#define BUDDY_SESSIONTASKDATA_H

#include "../SessionTaskData.h"
#include "../TaskStatistics.h"

namespace PdServ {
    class Session;
}

class Task;
class Signal;
struct Pdo;

class SessionTaskData: public PdServ::SessionTaskData {
    public:
        SessionTaskData(PdServ::Session* session, const Task* t,
                const char *album, unsigned int current,
                const struct app_properties*);

        const void *getValue(const Signal *) const;
        const PdServ::TaskStatistics* getTaskStatistics(const Task*) const;
        void newSignalData(unsigned int current);

    private:
        const char * const album;
        size_t const photoSize;
        size_t const statsOffset;

        const char *photo;
        PdServ::TaskStatistics stat;

        void updateStatistics();
};

#endif //BUDDY_SESSIONTASKDATA_H