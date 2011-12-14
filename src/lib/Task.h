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

#include "../Variable.h"
#include "../Task.h"
#include "../TaskStatistics.h"
//#include "ShmemDataStructures.h"

namespace PdServ {
    struct TaskStatistics;
}

class Main;
class Signal;
class SessionTaskData;

class Task: public PdServ::Task {
    public:
        Task(Main *main, double sampleTime, const char *name);
        ~Task();

        Main * const main;

        typedef std::vector<const Signal*> SignalVector;
        const SignalVector& getSignals() const;
        Signal* addSignal( unsigned int decimation,
                const char *path, PdServ::Variable::Datatype datatype,
                const void *addr, size_t n, const size_t *dim);

        size_t getShmemSpace(double t) const;

        SessionTaskData* newSession(PdServ::Session *session);

        void prepare(void *start, void *end);
        void rt_init();
        void nrt_init();
        void updateStatistics(
                double exec_time, double cycle_time, unsigned int overrun);
        void update(const struct timespec *);

        void subscribe(const Signal* s, bool insert);

        void pollPrepare( const Signal *, void *buf) const;
        bool pollFinished( const PdServ::Signal * const *s, size_t nelem,
                void * const *pollDest, struct timespec *t) const;

        size_t signalCount() const;
        void getSignalList(const Signal ** s, size_t *n,
                unsigned int *signalListId) const;

    private:
        mutable ost::Semaphore mutex;

        size_t signalTypeCount[4];
        size_t signalMemSize;

        PdServ::TaskStatistics taskStatistics;

        // Cache of the currently transferred signals
        const Signal **signalCopyList[4];

        // Position of a signal inside the copy list. Use the signal's index
        // to enter this array
        // Only allocated for non-realtime task
        unsigned int *signalPosition;

        SignalVector signalVector;

        unsigned int seqNo;
        unsigned int signalListId;

        // Pointer into shared memory used to communicate changes to the signal
        // copy list
        // The non-realtime thread writes using signalListWp whereas the
        // realtime task reads using signalListRp
        // This list is null terminated
        struct SignalList *signalList, *signalListEnd,
                          **signalListRp, **signalListWp;

        // Structure managed by the realtime thread containing a list of
        // signals which it has to copy into every PDO
        struct CopyList *copyList[4];

        // Process data communication. Points to shared memory
        struct Pdo *txMemBegin, *txPdo, **nextTxPdo;
        const void *txMemEnd;

        // Poll data communication. Points to shared memory.
        struct PollData *poll;
};

#endif // LIB_TASK_H
