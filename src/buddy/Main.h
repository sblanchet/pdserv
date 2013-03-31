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

#ifndef BUDDY_MAIN_H
#define BUDDY_MAIN_H

#include "../Main.h"
#include "fio_ioctl.h"
#include <set>
#include <log4cplus/logger.h>

namespace PdServ {
    class Session;
    class Signal;
    class Config;
}

class Parameter;
class Task;
class Event;
class EventQ;

class Main: public PdServ::Main {
    public:
        Main(const struct app_properties&,
                const PdServ::Config& config, int fd);

        void serve();

        int setParameter( const Parameter *p, const char* dataPtr,
                size_t len, struct timespec *) const;

    private:
        const struct app_properties &app_properties;
        log4cplus::Logger log;

        mutable ost::Semaphore paramMutex;

        int fd;
        int pid;

        Task *mainTask;

        bool getVariable(int type, size_t index, struct signal_info &si);

        char *parameterBuf;
        void *shmem;
        const char *photoAlbum;
        int readPointer;
        unsigned int *photoReady;
        unsigned int photoCount;

        size_t *readyList;

        EventQ *eventQ;

        typedef std::list<Event*> EventList;
        EventList events;

        typedef std::set<int> EventSet;
        EventSet eventSet;

//        void setupLogging(log4cpp::Category& log,
//                PdServ::Config const& config);
//        void setupTTYLog(log4cpp::Category& log);
//        void setupSyslog(log4cpp::Category& log,
//                PdServ::Config const& config);
//        void setupFileLog(log4cpp::Category& log,
//                PdServ::Config const& config);

        // Reimplemented from PdServ::Main
        void processPoll(unsigned int delay_ms,
                const PdServ::Signal * const *s, size_t nelem,
                void * const * pollDest, struct timespec *t) const;
        int gettime(struct timespec *) const;
        PdServ::Main::Events getEvents() const;
        void prepare(PdServ::Session *session) const;
        void cleanup(const PdServ::Session *session) const;
        const PdServ::Event *getNextEvent( const PdServ::Session* session,
                size_t *index, bool *state, struct timespec *t) const;
};

#endif // BUDDY_MAIN_H
