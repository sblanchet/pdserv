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

namespace PdServ {
    class Signal;
    class DataType;
}

class Parameter;
class Session;
class Signal;
class Event;
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

        Event* addEvent(const char *path, int prio,
                size_t nelem, const char **messages);
        bool setEvent(const Event* event,
                size_t element, bool state, const timespec* t) const;

        Parameter* addParameter( const char *path,
                unsigned int mode, const PdServ::DataType& datatype,
                void *addr, size_t n, const size_t *dim);
        int setParameter(const Parameter *param,
                size_t offset, size_t count, struct timespec *mtime) const;
        void parameterChanged(const PdServ::Session* session,
                const Parameter *param,
                const char *buf, size_t offset, size_t count) const;

        Signal* addSignal( Task *task, unsigned int decimation,
                const char *path, const PdServ::DataType& datatype,
                const void *addr, size_t n, const size_t *dim);

        static const double bufferTime;

    private:
        mutable ost::Semaphore mutex;

        typedef std::list<Task*> TaskList;
        TaskList task;

        int ipc_pipe[2];

        int pid;
        std::string configFile;
        PdServ::Config config;

        size_t tSampleMin;      // Minimal sample time in ms

        size_t shmem_len;
        void *shmem;

        /* Pointer to event states in shmem */
        struct EventData *eventData;

        /* Structure where event changes are written to in shmem */
        struct EventData **eventDataWp;   // Pointer to next write location
        struct EventData *eventDataStart; // First valid block
        struct EventData *eventDataEnd;   // Last valid block
        mutable ost::Semaphore eventMutex;

        struct SDOStruct *sdo;
        mutable ost::Semaphore sdoMutex;
        char *sdoData;

        int (* const rttime)(struct timespec*);

        typedef std::list<Event*> EventList;
        EventList events;

        typedef std::set<int> EventSet;
        EventSet eventSet;

        typedef std::set<std::string> VariableSet;
        VariableSet variableSet;

        typedef std::list<Parameter*> ParameterList;
        ParameterList parameters;

        int runForever();
        int readConfiguration();

        // Reimplemented from PdServ::Main
        int prefork_init();
        int postfork_rt_setup();
        int postfork_nrt_setup();
        void processPoll( unsigned int delay_ms,
                const PdServ::Signal * const *s, size_t nelem,
                void * const *pollDest, struct timespec *t) const;
        int gettime(struct timespec *) const;
        std::list<const PdServ::Task*> getTasks() const;
        std::list<const PdServ::Event*> getEvents() const;
        std::list<const PdServ::Parameter*> getParameters() const;
        void prepare(PdServ::Session *session) const;
        void cleanup(const PdServ::Session *session) const;
        const PdServ::Event *getNextEvent(const PdServ::Session* session,
                size_t *index, bool *state, struct timespec *t) const;
};

#endif // LIB_MAIN_H
