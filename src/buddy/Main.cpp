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
#include <sys/select.h>         // select()
#include <sys/signalfd.h>       // signalfd()
//#include <log4cplus/configurator.h>
//#include <log4cplus/syslogappender.h>
//#include <log4cplus/layout.h>
//#include <log4cplus/configurator.h>


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
    app_properties(p), log(log4cplus::Logger::getRoot()),
    config(config), pid(::getpid()), fd(fd),
    paramMutex(1), parameterBuf(0)
{
    setConfig(config[name]);
}

/////////////////////////////////////////////////////////////////////////////
int Main::serve()
{
    sigset_t mask;
    fd_set fds;
    int maxfd, sfd, rv;

    setupLogging();
    postfork_nrt_setup();
    startServers();

    // Set signal mask
    if (::sigemptyset(&mask)
            or ::sigaddset(&mask, SIGHUP)      // Reread configuration
            or ::sigaddset(&mask, SIGINT)      // Ctrl-C
            or ::sigaddset(&mask, SIGTERM)     // standard kill signal
            or ::sigprocmask(SIG_BLOCK, &mask, NULL)) { 
        rv = errno;
        LOG4CPLUS_FATAL_STR(log,
                LOG4CPLUS_TEXT("Failed to set correct signal mask"));
        goto out;
    }

    // Get a file descriptor for reading signals
    sfd = ::signalfd(-1, &mask, SFD_NONBLOCK);
    if (sfd == -1) {
        rv = errno;
        LOG4CPLUS_FATAL(log,
                LOG4CPLUS_TEXT("Getting signal file descriptor failed: ")
                << LOG4CPLUS_C_STR_TO_TSTRING(::strerror(errno)));
        goto out;
    }

    maxfd = (sfd > fd ? sfd : fd) + 1;

    FD_ZERO(&fds);
    while (true) {
        int n;

        FD_SET(fd, &fds);
        FD_SET(sfd, &fds);

        n = ::select(maxfd, &fds, 0, 0, 0);
        if (n <= 0) {
            // Some error occurred in select()
            rv = errno;
            LOG4CPLUS_FATAL(log,
                    LOG4CPLUS_TEXT("Received fatal error in select() = "
                        << errno));
            goto out;
        }

        if (FD_ISSET(fd, &fds)) {
            // New data pages
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

        if (FD_ISSET(sfd, &fds)) {
            // Signal received
            struct signalfd_siginfo fdsi;

            printf("Child %i got signal\n", pid);
            ssize_t s = ::read(sfd, &fdsi, sizeof(struct signalfd_siginfo));
            if (s != sizeof(struct signalfd_siginfo)) {
                rv = errno;
                LOG4CPLUS_FATAL_STR(log,
                        LOG4CPLUS_TEXT("Received incorrect length when "
                            "reading signal file descriptor"));
                goto out;
            }

            switch (fdsi.ssi_signo) {
                case SIGHUP:
                    LOG4CPLUS_INFO_STR(log,
                            LOG4CPLUS_TEXT("SIGUP received. Reloading config"));
                    config.reload();
                    setConfig(config[name]);
                    break;

                default:
                    LOG4CPLUS_INFO(log,
                            LOG4CPLUS_TEXT("Terminating on ")
                            << LOG4CPLUS_C_STR_TO_TSTRING(
                                strsignal(fdsi.ssi_signo))); 
                    stopServers();
                    return 0;
            }
        }
    }

out:
    LOG4CPLUS_FATAL(log,
            LOG4CPLUS_TEXT("Failed: ")
            << LOG4CPLUS_C_STR_TO_TSTRING(strerror(errno)));
    stopServers();
    return rv;
}

/////////////////////////////////////////////////////////////////////////////
int Main::postfork_nrt_setup()
{
    size_t shmem_len;
    size_t eventCount = 0;

    LOG4CPLUS_INFO(log,
            LOG4CPLUS_TEXT("Started new application server (pid=")
                << pid << LOG4CPLUS_TEXT(")"));
    LOG4CPLUS_INFO(log,
            LOG4CPLUS_TEXT("   Name: ") << app_properties.name);
    LOG4CPLUS_INFO(log,
            LOG4CPLUS_TEXT("   Block count: ") << app_properties.rtB_count);
    LOG4CPLUS_INFO(log,
            LOG4CPLUS_TEXT("   Block size: ") << app_properties.rtB_size);
    LOG4CPLUS_INFO(log,
            LOG4CPLUS_TEXT("   Parameter size: ") << app_properties.rtP_size);
    LOG4CPLUS_INFO(log,
            LOG4CPLUS_TEXT("   Number of tasks: ") << app_properties.num_tasks);
    LOG4CPLUS_INFO(log,
            LOG4CPLUS_TEXT("   Sample period (us): ")
            << app_properties.sample_period);
    LOG4CPLUS_INFO(log,
            LOG4CPLUS_TEXT("   Number of parameters: ")
            << app_properties.param_count);
    LOG4CPLUS_INFO(log,
            LOG4CPLUS_TEXT("   Number of signals: ")
            << app_properties.signal_count);

    // Go through the list of events and set up a map path->id
    // of paths and their corresponding id's to watch
    const PdServ::Config eventList = config["events"];
    typedef std::map<std::string, PdServ::Config> EventMap;
    EventMap eventMap;
    PdServ::Config eventConf;
    size_t i;
    for (i = 0, eventConf = eventList[i];
            eventConf; eventConf = eventList[++i]) {

        std::string subsys = eventConf["subsystem"].toString();
        eventMap[subsys] = eventConf;
        LOG4CPLUS_TRACE(log,
                LOG4CPLUS_TEXT("Added event " << subsys.c_str()));
    }

    // Get a copy of the parameters
    parameterBuf = new char[app_properties.rtP_size];
    if (::ioctl(fd, GET_PARAM, parameterBuf))
        goto out;

    char path[app_properties.variable_path_len+1];
    struct signal_info si;
    si.path = path;
    si.path_buf_len = app_properties.variable_path_len + 1;
    LOG4CPLUS_DEBUG_STR(log, LOG4CPLUS_TEXT("Adding parameters:"));
    for (size_t i = 0; i < app_properties.param_count; i++) {

        si.index = i;
        if (::ioctl(fd, GET_PARAM_INFO, &si) or !si.dim[0]) {
            LOG4CPLUS_WARN(log,
                    LOG4CPLUS_TEXT("Getting parameter properties for ")
                    << i << LOG4CPLUS_TEXT(" failed."));
            continue;
        }

        Parameter *p = new Parameter( this, parameterBuf + si.offset,
                SignalInfo(app_properties.name, &si));
        parameters.push_back(p);
        LOG4CPLUS_DEBUG_STR(log, LOG4CPLUS_STRING_TO_TSTRING(p->path));
    }

    photoReady = new unsigned int[app_properties.rtB_count];
    std::fill_n(photoReady, app_properties.rtB_count, 0);
    photoCount = 0;

    shmem_len = app_properties.rtB_size * app_properties.rtB_count;
    shmem = ::mmap(0, shmem_len, PROT_READ, MAP_PRIVATE, fd, 0);
    if (shmem == MAP_FAILED)
        goto out;
    LOG4CPLUS_INFO(log,
            LOG4CPLUS_TEXT("Mapped shared memory segment with ") << shmem_len
            << LOG4CPLUS_TEXT(" and ") << app_properties.rtB_count
            << LOG4CPLUS_TEXT(" snapshots."));

    photoAlbum = reinterpret_cast<const char *>(shmem);

    mainTask = new Task(this, 1.0e-6 * app_properties.sample_period,
            photoReady, photoAlbum, &app_properties);
    task.push_back(mainTask);
    LOG4CPLUS_DEBUG(log,
            LOG4CPLUS_TEXT("Added Task with sample time ")
            << mainTask->sampleTime);

    LOG4CPLUS_DEBUG_STR(log, LOG4CPLUS_TEXT("Adding signals:"));
    for (size_t i = 0; i < app_properties.signal_count; i++) {

        si.index = i;
        if (ioctl(fd, GET_SIGNAL_INFO, &si) or !si.dim[0]) {
            LOG4CPLUS_WARN(log,
                    LOG4CPLUS_TEXT("Getting signals properties for ")
                    << i << LOG4CPLUS_TEXT(" failed"));
            continue;
        }

        const Signal *s =
            mainTask->addSignal( SignalInfo(app_properties.name, &si));

        // Check whether this signal is actually an event
        EventMap::iterator it = eventMap.find(s->path);
        if (it != eventMap.end()
                and s->dtype.primary() == s->dtype.double_T) {

            Event *e = new Event(s, events.size(),
                    it->second.toUInt(), it->second["priority"].toString());
            events.push_back(e);
            eventCount += e->nelem;
            LOG4CPLUS_DEBUG(log,
                    LOG4CPLUS_TEXT("Watching as event: ")
                    << LOG4CPLUS_STRING_TO_TSTRING(s->path.c_str()));

            // Set event message
            e->message = new const char *[s->dim.nelem];
            PdServ::Config messageList = it->second["message"];
            if (s->dim.nelem > 1) {
                for (size_t i = 0; i < s->dim.nelem; ++i) {
                    std::string message = messageList[i].toString();
                    if (message.empty()) {
                        e->message[i] = 0;
                        continue;
                    }
                    char *s = new char[message.size() + 1];
                    e->message[i] = s;
                    std::copy(message.begin(), message.end(), s);
                    s[message.size()] = 0;
                }
            }
            else {
                std::string message = messageList.toString();

                if (message.empty())
                    e->message[0] = 0;
                else {
                    char *s = new char[message.size() + 1];
                    e->message[0] = s;
                    std::copy(message.begin(), message.end(), s);
                    s[message.size()] = 0;
                }
            }

            // Reset config to indicate that the path was found
            it->second = PdServ::Config();
        }

        LOG4CPLUS_DEBUG_STR(log, LOG4CPLUS_STRING_TO_TSTRING(s->path));
    }

    // Report whether some events were not discovered
    for (EventMap::const_iterator it = eventMap.begin();
            it != eventMap.end(); ++it) {
        if (it->second)
            LOG4CPLUS_WARN(log,
                    LOG4CPLUS_TEXT("Event ")
                    << LOG4CPLUS_STRING_TO_TSTRING(it->first)
                    << LOG4CPLUS_TEXT(" is not found"));
    }

    eventQ = new EventQ(eventCount * 2, photoAlbum, &app_properties);

    readPointer = ioctl(fd, RESET_BLOCKIO_RP);

    return 0;

out:
    LOG4CPLUS_FATAL(log,
            LOG4CPLUS_TEXT("Failed: ")
            << LOG4CPLUS_C_STR_TO_TSTRING(strerror(errno)));
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
int Main::setParameter( const Parameter * /*param*/, const char *dataPtr,
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
