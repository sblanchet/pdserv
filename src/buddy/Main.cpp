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
#include "Event.h"
#include "EventQ.h"
#include "Parameter.h"
#include "../Config.h"
#include "../Session.h"

/////////////////////////////////////////////////////////////////////////////
struct SessionData {
    size_t eventReadPointer;
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Main::Main(const struct app_properties& p,
        const PdServ::Config& config, int fd):
    PdServ::Main(p.name, p.version),
    app_properties(p),
    log(log4cpp::Category::getInstance(p.name)),
    paramMutex(1), parameterBuf(0)
{
    pid = ::getpid();
    size_t shmem_len;
    size_t eventCount = 0;

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

    // Go through the list of events and set up a map path->id
    // of paths and their corresponding id's to watch
    const PdServ::Config eventList = config[name]["events"];
    typedef std::map<std::string, PdServ::Config> EventMap;
    EventMap eventMap;
    PdServ::Config eventConf;
    size_t i;
    for (i = 0, eventConf = eventList[i];
            eventConf; eventConf = eventList[++i]) {

        size_t id = eventConf.toUInt();

        std::string subsys =
            eventList[eventConf.toString()]["subsystem"].toString();
        if (!subsys.empty() and id) {
            eventMap[subsys] = eventConf;
            log.debug("Added event %s = %zu", subsys.c_str(), id);
        }
    }

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

    photoReady = new unsigned int[app_properties.rtB_count];
    std::fill_n(photoReady, app_properties.rtB_count, 0);
    photoCount = 0;

    shmem_len = app_properties.rtB_size * app_properties.rtB_count;
    shmem = ::mmap(0, shmem_len, PROT_READ, MAP_PRIVATE, fd, 0);
    if (shmem == MAP_FAILED)
        goto out;
    log.info("Mapped shared memory segment with %zu bytes and %zu snapshots",
            shmem_len, app_properties.rtB_count);

    photoAlbum = reinterpret_cast<const char *>(shmem);

    mainTask = new Task(this, 1.0e-6 * app_properties.sample_period,
            photoReady, photoAlbum, &app_properties);
    task.push_back(mainTask);
    log.debug("Added Task with sample time %f", mainTask->sampleTime);

    log.debug("Adding signals:");
    for (size_t i = 0; i < app_properties.signal_count; i++) {

        si.index = i;
        if (ioctl(fd, GET_SIGNAL_INFO, &si) or !si.dim[0]) {
            log.warn("Getting signals properties for %i failed.", i);
            continue;
        }

        const Signal *s =
            mainTask->addSignal( SignalInfo(app_properties.name, &si));

        // Check whether this signal is actually an event
        EventMap::iterator it = eventMap.find(s->path);
        if (it != eventMap.end()
                and s->dtype.primary() == s->dtype.double_T) {
            Event *e = new Event(s, events.size(), it->second);
            events.push_back(e);
            eventCount += e->nelem;
            log.info("Watching as event: %s", s->path.c_str());

            PdServ::Config config = it->second;

            // Set event message
            e->message = config["message"].toString();

            // Set index offset
            e->indexOffset = config["indexoffset"].toInt();

            // Setup index mapping if present
            size_t i = 0;
            PdServ::Config indexMapping = config["indexmapping"];
            for (PdServ::Config map = indexMapping[i];
                    map; map = indexMapping[++i])
                e->indexMap[map.toUInt()] =
                    indexMapping[map.toString()].toString();

            // Reset config to indicate that the path was found
            it->second = PdServ::Config();
        }

        log.debug("   %zu %s", i, s->path.c_str());
    }

    // Report whether some events were not discovered
    for (EventMap::const_iterator it = eventMap.begin();
            it != eventMap.end(); ++it) {
        if (it->second)
            log.warn("Event %s is not found", it->first.c_str());
    }

    eventQ = new EventQ(eventCount * 2, photoAlbum, &app_properties);

    readPointer = ioctl(fd, RESET_BLOCKIO_RP);

    startServers(config);

    return;

out:
    log.crit("Failed: %s (%i)", strerror(errno), errno);
    ::exit(errno);
}

/////////////////////////////////////////////////////////////////////////////
void Main::serve()
{
    while (true) {
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
            continue;
        }

        struct data_p data_p;
        ioctl(fd, GET_WRITE_PTRS, &data_p);

        if (data_p.wp == -ENOSPC) {
            readPointer = ioctl(fd, RESET_BLOCKIO_RP);
            continue;
        }

        for (int i = readPointer; i < data_p.wp; ++i) {

            // Mark photo as ready
            photoReady[i] = photoCount++;

            // Go through event list to see if something happened
            for (EventList::const_iterator it = events.begin();
                    it != events.end(); ++it)
                eventQ->test(*it, i);
        }

        // Tell kernel that reading has finished
        readPointer = ioctl(fd, SET_BLOCKIO_RP, data_p.wp);
    }

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
int Main::setParameter( const Parameter *param, const char *dataPtr,
        size_t count, struct timespec *mtime) const
{
    ost::SemaphoreLock lock(paramMutex);
    struct param_change delta;

    delta.rtP = parameterBuf;
    delta.pos = dataPtr - parameterBuf;
    delta.len = count;
    delta.count = 0;
    gettime(mtime);

    if (ioctl(fd, CHANGE_PARAM, &delta)) {
        return errno;
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
PdServ::Main::Events Main::getEvents() const
{
    return Events(events.begin(), events.end());
}

/////////////////////////////////////////////////////////////////////////////
void Main::prepare(PdServ::Session *session) const
{
    session->data = new SessionData;
    eventQ->initialize(session->data->eventReadPointer);
}

/////////////////////////////////////////////////////////////////////////////
void Main::cleanup(const PdServ::Session *session) const
{
    delete session->data;
}

/////////////////////////////////////////////////////////////////////////////
const PdServ::Event* Main::getNextEvent (const PdServ::Session* session,
        size_t *index, bool *state, struct timespec *t) const
{
    return eventQ->getNextEvent(
            session->data->eventReadPointer, index, state, t);
}
