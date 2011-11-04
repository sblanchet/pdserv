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

#include <cstddef>
#include <set>
#include <map>
#include <cc++/thread.h>

#include "pdserv/etl_data_info.h"
#include "ShmemDataStructures.h"

class Main;
class Signal;

namespace PdServ {
    class Session;
    class Signal;
}

class Task {
    public:
        Task(Main *main, double sampleTime, const char *name);
        ~Task();

        const double sampleTime;
        Main * const main;

        void newSignal(const Signal*);

        size_t getShmemSpace(double t) const;

        void prepare(void *start, void *end);
        void rt_init();
        void nrt_init();
        void update(const struct timespec *);

        void subscribe(const Signal* s, bool insert) const;

        void pollPrepare( const Signal *) const;
        bool pollFinished() const;

    private:
        mutable ost::Semaphore mutex;

        size_t signalCount[4];
        size_t signalMemSize;
        size_t pdoMem;

        unsigned int seqNo;

        struct CopyList *copyList[4];
        struct SignalList *signalList, *signalListEnd,
                          **signalListRp, **signalListWp;

        struct TxPdo *txMemBegin, *txPdo, **nextTxPdo;
        const void *txMemEnd;

        mutable unsigned int pollId;
        mutable unsigned int pollCount;
        mutable char *pollPtr;

        struct PollData *poll;
        char *pollData;
};

#endif // LIB_TASK_H
