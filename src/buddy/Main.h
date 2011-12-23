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

#include "../Main.h"
#include "fio_ioctl.h"

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
class BuddyConfigRef;

class Main: public PdServ::Main {
    public:
        Main();

        void serve(const BuddyConfigRef& config, int fd,
                struct app_properties const * app_properties);
        int setParameter(const Parameter *p, size_t startIndex,
                size_t nelem, const char *data,
                struct timespec *) const;

    private:
        mutable ost::Semaphore paramMutex;

        int fd;
        int pid;

        Task *mainTask;
        struct app_properties const * app_properties;

        bool getVariable(int type, size_t index, struct signal_info &si);

        char *parameterBuf;
        void *shmem;
        const char *photoAlbum;
        int readPointer;
        unsigned int *photoReady;
        unsigned int photoCount;

        size_t *readyList;

        // Reimplemented from PdServ::Main
        void processPoll(unsigned int delay_ms,
                const PdServ::Signal * const *s, size_t nelem,
                void * const * pollDest, struct timespec *t) const;
        int gettime(struct timespec *) const;
        PdServ::SessionShadow *newSession(PdServ::Session *) const;
};

#endif // BUDDY_MAIN_H
