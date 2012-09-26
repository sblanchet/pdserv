/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdserv library.
 *
 *  The pdserv library is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or (at
 *  your option) any later version.
 *
 *  The pdserv library is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with the pdserv library. If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#ifndef LIB_MAIN_H
#define LIB_MAIN_H

#include <set>

#include "../Main.h"
#include "../Variable.h"
#include "../Config.h"

#include <cc++/thread.h>

namespace PdServ {
    class Signal;
    class DataType;
}

class Parameter;
class Signal;
class Task;

class Main: public PdServ::Main {
    public:
        Main( const char *name, const char *version,
                int (*gettime)(struct timespec*));
        ~Main();

        void setConfigFile(const char *file);

        int run();
        void getParameters(Task *, const struct timespec *) const;

        Task* addTask(double sampleTime, const char *name);
        Task* getTask(size_t index) const;

        Parameter* addParameter( const char *path,
                unsigned int mode, const PdServ::DataType& datatype,
                void *addr, size_t n, const size_t *dim);

        Signal* addSignal( Task *task, unsigned int decimation,
                const char *path, const PdServ::DataType& datatype,
                const void *addr, size_t n, const size_t *dim);
        int setParameter(const Parameter *p, size_t offset,
                size_t count, const char *data,
                struct timespec *) const;

        static const double bufferTime;

    private:
        mutable ost::Semaphore mutex;

        std::string configFile;
        PdServ::Config config;

        int pid;

        size_t shmem_len;
        void *shmem;

        struct SDOStruct *sdo;
        mutable ost::Semaphore sdoMutex;
        char *sdoData;

        int (* const rttime)(struct timespec*);

        typedef std::set<std::string> VariableSet;
        VariableSet variableSet;

        int daemonize();
        void consoleLogging();
        void configureLogging(const PdServ::Config&);

        // Reimplemented from PdServ::Main
        void processPoll( unsigned int delay_ms,
                const PdServ::Signal * const *s, size_t nelem,
                void * const *pollDest, struct timespec *t) const;
        int gettime(struct timespec *) const;
};

#endif // LIB_MAIN_H
