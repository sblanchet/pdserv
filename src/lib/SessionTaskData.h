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

#ifndef LIBSESSIONTASKDATA_H
#define LIBSESSIONTASKDATA_H

#include <vector>

#include "../SessionTaskData.h"

namespace PdServ {
    class Session;
    class Signal;
    class TaskStatistics;
}

class Task;
class Signal;
struct Pdo;

class SessionTaskData: public PdServ::SessionTaskData {
    public:
        SessionTaskData(Task* t, PdServ::Session* session,
                struct Pdo *pdo, size_t signalCount,
                unsigned int signalListId, const Signal * const *signalList,
                size_t nelem);

        void rxPdo();
        void newSignalList( unsigned int signalListId,
                const Signal * const *sp, size_t n);
        void newSignalData( unsigned int signalListId, 
                const PdServ::TaskStatistics *stats, const char *buf);
        const char *getValue(const PdServ::Signal *) const;

    private:
        Task * const task;
        PdServ::Session * const session;

        std::vector<size_t> signalPosition;

        unsigned int signalListId;
        const char *signalBuffer;

        struct Pdo *pdo;

        void loadSignalList(const Signal * const *sp, size_t n);
};

#endif //LIBSESSIONTASKDATA_H
