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

#include <map>

#include "../Main.h"
#include "../Variable.h"

#include <cc++/thread.h>

namespace PdServ {
    class Session;
    class Signal;
    class SessionShadow;
}

class Parameter;
class Signal;
class Task;
class SessionShadow;

class Main: public PdServ::Main {
    public:
        Main( const char *name, const char *version,
                int (*gettime)(struct timespec*));
        ~Main();

        int configFile(const std::string& name);

        int run();
        void getParameters(Task *, const struct timespec *) const;

        Task* addTask(double sampleTime, const char *name);
        Task* getTask(size_t index) const;

        Parameter* addParameter( const char *path,
                unsigned int mode, PdServ::Variable::Datatype datatype,
                void *addr, size_t n, const unsigned int *dim);

        Signal* addSignal( Task *task, unsigned int decimation,
                const char *path, PdServ::Variable::Datatype datatype,
                const void *addr, size_t n, const unsigned int *dim);
        int setParameter(const Parameter *p, size_t startIndex,
                size_t nelem, const char *data,
                struct timespec *) const;

        static const double bufferTime;

    private:
        mutable ost::Semaphore mutex;

        int pid;

        size_t shmem_len;
        void *shmem;

        struct SDOStruct *sdo;
        mutable ost::Semaphore sdoMutex;
        char *sdoData;

        int (* const rttime)(struct timespec*);

        int daemonize();

        // Reimplemented from PdServ::Main
        void processPoll(size_t delay_ms,
                const PdServ::Signal * const *s, size_t nelem,
                void * const *pollDest, struct timespec *t) const;
        int gettime(struct timespec *) const;
        PdServ::SessionShadow *newSession(PdServ::Session *) const;
};

#endif // LIB_MAIN_H
