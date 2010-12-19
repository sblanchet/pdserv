/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdcomserv package.
 *
 *  pdcomserv is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pdcomserv is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pdcomserv. See COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#ifndef LIB_TASK_H
#define LIB_TASK_H

#include <cstddef>
#include <set>
#include <map>
#include <cc++/thread.h>

#include "../Task.h"
#include "rtlab/etl_data_info.h"
#include "ShmemDataStructures.h"

class Main;
class Signal;
class Receiver;

class Task: public HRTLab::Task {
    public:
        Task(Main *main, unsigned int tid, double sampleTime);
        ~Task();

        void newSignal(const Signal*);

        size_t getShmemSpace(double t) const;
        void prepare(void *start, void *end);
        void rt_init();
        void nrt_init();

        size_t subscribe(const HRTLab::Session*,
                const HRTLab::Signal* const *, size_t n) const;
        size_t unsubscribe(const HRTLab::Session*,
                const HRTLab::Signal* const * = 0, size_t n = 0) const;

        void txPdo(const struct timespec *);

        Receiver *newReceiver();
        void receiverDeleted(Receiver *);

    private:
        Main * const main;

        typedef std::set<Receiver*> ReceiverSet;
        ReceiverSet receiverSet;

        mutable ost::Semaphore mutex;

        size_t signalCount;
        size_t signalMemSize;
        size_t pdoMem;

        typedef std::set<const Signal*> SignalSet;
        mutable SignalSet subscriptionSet[4];

        typedef std::map<const HRTLab::Session*, SignalSet>
            SessionSubscription;
        mutable SessionSubscription sessionSubscription;

        unsigned int seqNo;

        unsigned int copyListId;
        struct CopyList {
            const char *src;
            size_t len;
        } *copyList;

        struct SignalList {
            unsigned int requestId;
            unsigned int reply;
            size_t pdoMemSize;
            unsigned int count;
            struct CopyList list[];
        };
        SignalList *activeSet;

        TxFrame *txMemBegin, *txFrame, **nextTxFrame;
        const void *txMemEnd;

        void newSignalList(size_t n) const;
};

#endif // LIB_TASK_H
