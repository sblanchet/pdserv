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

#ifndef LIB_MAIN_H
#define LIB_MAIN_H

#include "../Main.h"

#include <cc++/thread.h>
#include <set>

class Receiver;
class Parameter;
class Signal;
class Task;

class Main: public PdServ::Main {
    public:
        Main(int argc, const char *argv[],
                const char *name, const char *version,
                double baserate,
                unsigned int nst, const unsigned int decimation[],
                int (*gettime)(struct timespec*));
        ~Main();

        Task ** const task;

        int init();
        void update(int st, const struct timespec *time) const;
        int setParameter(const Parameter *p, size_t startIndex,
                size_t nelem, const char *data,
                struct timespec *) const;

//        void newSignalList(unsigned int listId,
//                const PdServ::Signal * const *, size_t n) const;

        static const double bufferTime;

    private:
        mutable ost::Semaphore mutex;

        int pid;

        size_t shmem_len;
        void *shmem;

        struct SDOStruct {
            unsigned int reqId;
            unsigned int replyId;
            enum {PollSignal, WriteParameter} type;
            int errorCode;
            unsigned int startIndex;
            unsigned int count;
            struct timespec time;
            union {
                const Signal *signal[];
                const Parameter *parameter;
            };
        } *sdo;
        mutable ost::Semaphore sdoMutex;
        struct timespec *sdoTaskTime;
        char *sdoData;

        // Methods used by the real-time process to interpret inbox
        // instructions from the clients
        bool processSdo(unsigned int tid, const struct timespec *time) const;
//
//        static int localtime(struct timespec *);

        int (* const rttime)(struct timespec*);

        // Reimplemented from PdServ::Main
        int gettime(struct timespec *) const;
        void rxPdo(PdServ::Session *);
        void getValues(const PdServ::Signal * const *s,
                size_t nelem, char *buf, struct timespec * = 0) const;
        PdServ::Receiver *newReceiver(unsigned int tid);
        void subscribe(PdServ::Session *,
                const PdServ::Signal * const *, size_t n) const;
        void unsubscribe(PdServ::Session *,
                const PdServ::Signal * const * = 0, size_t n = 0) const;
};

#endif // LIB_MAIN_H
