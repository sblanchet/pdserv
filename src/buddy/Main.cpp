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

#include "config.h"
#include "../Debug.h"

#include <sstream>
#include <cerrno>
#include <cstdlib>              // exit()
#include <sys/ioctl.h>          // ioctl()
#include <unistd.h>             // fork()
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/wait.h>           // waitpid()

#include <log4cpp/OstreamAppender.hh>
#include <log4cpp/SimpleLayout.hh>
#include <log4cpp/Category.hh>

#include "Main.h"
#include "Task.h"
#include "Signal.h"
#include "Parameter.h"
#include "SessionShadow.h"
#include "../Config.h"
#include "../Session.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Main::Main(const struct app_properties& p):
    PdServ::Main(p.name, p.version),
    app_properties(p),
    log(log4cpp::Category::getInstance(p.name)),
    paramMutex(1), parameterBuf(0)
{
    pid = ::getpid();
}

/////////////////////////////////////////////////////////////////////////////
void Main::serve(const PdServ::Config& config, int fd)
{
    size_t shmem_len;

    this->fd = fd;

    log.notice("Started new application server (%i)", pid);
    log.info("   Block count: %zu", app_properties.rtB_count);
    log.info("   Block size: %zu", app_properties.rtB_size);
    log.info("   Parameter size: %zu", app_properties.rtP_size);
    log.info("   Number of sample times: %zu", app_properties.num_st);
    log.info("   Number of tasks: %zu", app_properties.num_tasks);
    log.info("   Sample period: %luus", app_properties.sample_period);
    log.info("   Number of parameters: %zu", app_properties.param_count);
    log.info("   Number of signals: %zu", app_properties.signal_count);

    // Get a copy of the parameters
    parameterBuf = new char[app_properties.rtP_size];
    if (ioctl(fd, GET_PARAM, parameterBuf))
        goto out;

    char path[app_properties.variable_path_len+1];
    struct signal_info si;
    si.path = path;
    si.path_buf_len = app_properties.variable_path_len + 1;
    log.debug("Adding parameters:");
    for (size_t i = 0; i < app_properties.param_count; i++) {

        si.index = i;
        if (ioctl(fd, GET_PARAM_INFO, &si) or !si.dim[0]) {
            log.warn("Getting parameter properties for %i failed.", i);
            continue;
        }

        Parameter *p = new Parameter( this, parameterBuf + si.offset,
                SignalInfo(app_properties.name, &si));
        parameters.push_back(p);
        log.debug("   %zu %s", i, p->path.c_str());
    }
    
    mainTask = new Task(this, 1.0e-6 * app_properties.sample_period);
    task.push_back(mainTask);
    log.debug("Added Task with sample time %f", mainTask->sampleTime);

    log.debug("Adding signals:");
    for (size_t i = 0; i < app_properties.signal_count; i++) {

        si.index = i;
        if (ioctl(fd, GET_SIGNAL_INFO, &si) or !si.dim[0]) {
            log.warn("Getting signals properties for %i failed.", i);
            continue;
        }

        const PdServ::Signal *s =
            mainTask->addSignal( SignalInfo(app_properties.name, &si));

        log.debug("   %zu %s", i, s->path.c_str());
    }

    shmem_len = app_properties.rtB_size * app_properties.rtB_count;
    shmem = ::mmap(0, shmem_len, PROT_READ, MAP_PRIVATE, fd, 0);
    if (shmem == MAP_FAILED)
        goto out;
    log.info("Mapped shared memory segment with %zu bytes and %zu snapshots",
            shmem_len, app_properties.rtB_count);

    photoAlbum = reinterpret_cast<const char *>(shmem);

    readPointer = ioctl(fd, RESET_BLOCKIO_RP);

    photoReady = new unsigned int[app_properties.rtB_count];
    std::fill_n(photoReady, app_properties.rtB_count, 0);
    photoCount = 0;

    startServers(config);

    do {
        fd_set fds;
        int n;

        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        n = ::select(fd + 1, &fds, 0, 0, 0);
        if (n < 0) {
            // Some error occurred in select()
            log.crit("Received fatal error in select() = %i", -errno);
            goto out;
        }
        else if (!n) {
            // Signal was caught, check whether the children are still there
            log.warn("Received misterious interrupt during select()");
        }
        else {
            struct data_p data_p;
            ioctl(fd, GET_WRITE_PTRS, &data_p);

            if (data_p.wp == -ENOSPC) {
                readPointer = ioctl(fd, RESET_BLOCKIO_RP);
            }
            else {
                for (int i = readPointer; i < data_p.wp; ++i)
                    photoReady[i] = photoCount++;
                readPointer = ioctl(fd, SET_BLOCKIO_RP, data_p.wp);
            }
        }
    } while (true);
    
out:
    log.crit("Failed: %s (%i)", strerror(errno), errno);
    ::exit(errno);
}

/////////////////////////////////////////////////////////////////////////////
void Main::processPoll(unsigned int /*delay_ms*/,
        const PdServ::Signal * const *s, size_t nelem,
        void * const * pollDest, struct timespec *) const
{
    const char *data = photoAlbum + readPointer * app_properties.rtB_size;

    for (size_t i = 0; i < nelem; ++i) {
        const Signal *signal = dynamic_cast<const Signal *>(s[i]);
        if (signal)
            signal->info.read( pollDest[i], data + signal->offset);
    }
}

/////////////////////////////////////////////////////////////////////////////
int Main::gettime(struct timespec *ts) const
{
    struct timeval tv;
    if (gettimeofday(&tv, 0))
        return -1;

    ts->tv_sec = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000;

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
PdServ::SessionShadow *Main::newSession(PdServ::Session *session) const
{
    return new SessionShadow(session, mainTask, readPointer, photoReady,
            photoAlbum, &app_properties);
}

/////////////////////////////////////////////////////////////////////////////
int Main::setParameter( const Parameter *param, size_t startIndex,
        size_t count, const char *data, struct timespec *mtime) const
{
    ost::SemaphoreLock lock(paramMutex);
    struct param_change delta;

    delta.rtP = parameterBuf;
    delta.pos = data - parameterBuf + startIndex * param->elemSize;
    delta.len = count * param->elemSize;                                                        
    delta.count = 0;
    gettime(mtime);

    if (ioctl(fd, CHANGE_PARAM, &delta)) {
        return errno;
    }

    return 0;
}
