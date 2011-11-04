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
#include <list>

class Parameter;
class Signal;
class Task;

class Main: public PdServ::Main {
    public:
        Main(int argc, const char *argv[],
                const char *name, const char *version,
                int (*gettime)(struct timespec*));
        ~Main();

        int init();
        void addTask(Task *);

        void addParameter(const Parameter *p);
//        int setParameter(const Parameter *p, size_t startIndex,
//                size_t nelem, const char *data,
//                struct timespec *) const;

//        void newSignalList(unsigned int listId,
//                const PdServ::Signal * const *, size_t n) const;

        void setPollDelay(unsigned int ms) const;

        static const double bufferTime;

    private:
        mutable ost::Semaphore mutex;

        int pid;

        mutable unsigned int pollDelay;

        size_t shmem_len;
        void *shmem;

        struct SDOStruct *sdo;
        mutable ost::Semaphore sdoMutex;
        struct timespec *sdoTaskTime;
        char *sdoData;

        typedef std::list<Task*> TaskList;
        TaskList task;
// 
//         // Methods used by the real-time process to interpret inbox
//         // instructions from the clients
//         bool processSdo(Task *task, const struct timespec *time) const;
// //
// //        static int localtime(struct timespec *);
// 
        int (* const rttime)(struct timespec*);
// 
        // Reimplemented from PdServ::Main
        void processPoll() const;
        int gettime(struct timespec *) const;
};

#endif // LIB_MAIN_H
