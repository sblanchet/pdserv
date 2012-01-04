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

#ifndef BUDDY_MAIN_H
#define BUDDY_MAIN_H

#include <map>
#include <set>
#include <log4cpp/Category.hh>

#include "../Main.h"
#include "fio_ioctl.h"

#include <cc++/thread.h>

namespace log4cpp {
    class Category;
}

namespace PdServ {
    class Session;
    class Signal;
    class Config;
    class ProcessParameter;
    class SessionShadow;
}

class Parameter;
class Signal;
class Task;
class SessionShadow;

class Main: public PdServ::Main {
    public:
        Main(const struct app_properties&);

        void serve(const PdServ::Config& config, int fd);

        int setParameter( const Parameter *p, size_t startIndex,
                size_t nelem, const char *data, struct timespec *) const;

    private:
        const struct app_properties &app_properties;
        log4cpp::Category &log;

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

        void setupLogging(log4cpp::Category& log, 
                PdServ::Config const& config);
        void setupTTYLog(log4cpp::Category& log);
        void setupSyslog(log4cpp::Category& log, 
                PdServ::Config const& config);
        void setupFileLog(log4cpp::Category& log, 
                PdServ::Config const& config);

        // Reimplemented from PdServ::Main
        void processPoll(unsigned int delay_ms,
                const PdServ::Signal * const *s, size_t nelem,
                void * const * pollDest, struct timespec *t) const;
        int gettime(struct timespec *) const;
        PdServ::SessionShadow *newSession(PdServ::Session *) const;
};

#endif // BUDDY_MAIN_H
