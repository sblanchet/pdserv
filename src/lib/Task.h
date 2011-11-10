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

#ifndef LIB_TASK_H
#define LIB_TASK_H

#include <vector>
#include <map>
#include <cstddef>
#include <cc++/thread.h>

#include "pdserv/etl_data_info.h"
#include "ShmemDataStructures.h"
#include "../Task.h"

class Main;
class Signal;
class SessionTaskData;

class Task: public PdServ::Task {
    public:
        Task(Main *main, double sampleTime, const char *name);
        ~Task();

        Main * const main;

        Signal* addSignal( unsigned int decimation,
                const char *path, enum si_datatype_t datatype,
                const void *addr, size_t n, const unsigned int *dim);

        size_t getShmemSpace(double t) const;

        void prepare(void *start, void *end);
        void rt_init();
        void nrt_init();
        void update(const struct timespec *,
                double exec_time, double cycle_time);
        bool rxPdo(struct Pdo **, SessionTaskData *);

        void subscribe(const Signal* s, bool insert) const;
        SessionTaskData *newSession(PdServ::Session *s);
        void endSession(PdServ::Session *s);

        void pollPrepare( const Signal *, char *buf) const;
        bool pollFinished( const PdServ::Signal * const *s, size_t nelem,
                char * const *pollDest, struct timespec *t) const;

    private:
        mutable ost::Semaphore mutex;

        size_t signalCount[4];
        size_t signalMemSize;
        size_t pdoMem;

        std::map<const PdServ::Session*, SessionTaskData*> sessionMap;
        std::vector<size_t> signalPosition;

        unsigned int seqNo;
        unsigned int signalListId;

        struct CopyList *copyList[4];
        struct SignalList *signalList, *signalListEnd,
                          **signalListRp, **signalListWp;

        struct Pdo *txMemBegin, *txPdo, **nextTxPdo;
        const void *txMemEnd;

        struct PollData *poll;
        char *pollData;
};

#endif // LIB_TASK_H
