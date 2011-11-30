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

#include "config.h"
#include "../Debug.h"

#include <sstream>
#include <cerrno>
#include <cstdlib>              // exit()
#include <sys/ioctl.h>          // ioctl()
#include <unistd.h>             // fork()
// #include <sys/types.h>          // kill()
#include <sys/mman.h>
#include <sys/select.h>

#include "Main.h"
#include "Signal.h"
#include "SessionShadow.h"
#include "../Session.h"
#include "Task.h"
#include "Parameter.h"

//ost::CommandOptionNoArg traditional(
//        "traditional", "t", "Traditional MSR", false);

/////////////////////////////////////////////////////////////////////////////
Main::Main(int argc, const char **argv,
        int instance, const char *device_node):
    PdServ::Main(argc, argv, "", ""),
    paramMutex(1), parameterBuf(0)
{
    pid = fork();
    if (pid < 0) {
        throw pid;
    }
    else if (pid) {
        return;
    }
    debug() << "fork()" << pid;

    std::ostringstream device;

    device << device_node << instance + 1;

    fd = open(device.str().c_str(), O_NONBLOCK | O_RDONLY);
    if (fd < 0)
        goto out;
    debug() << "opened" << device.str() << '=' << fd;

    if (ioctl(fd, GET_APP_PROPERTIES, &app_properties))
        goto out;

    name = app_properties.name;
    version = app_properties.version;

    // Get a copy of the parameters
    parameterBuf = new char[app_properties.rtP_size];
    if (ioctl(fd, GET_PARAM, parameterBuf))
        goto out;

    char path[app_properties.variable_path_len+1];
    struct signal_info si;
    si.path = path;
    si.path_buf_len = app_properties.variable_path_len + 1;
    for (size_t i = 0; i < app_properties.param_count; i++) {

        si.index = i;
        if (ioctl(fd, GET_PARAM_INFO, &si) or !si.dim[0])
            continue;

        Parameter *p = new Parameter( this, parameterBuf + si.offset,
                SignalInfo(app_properties.name, &si));
        parameters.push_back(p);
        variableMap[p->path] = p;
    }
    debug() << app_properties.param_count << "parameters";

    mainTask = new Task(this, 1.0e-6 * app_properties.sample_period);
    task.push_back(mainTask);
    for (size_t i = 0; i < app_properties.signal_count; i++) {

        si.index = i;
        if (ioctl(fd, GET_SIGNAL_INFO, &si) or !si.dim[0])
            continue;

        //debug() << app_properties.name << '|' << si.path << '|' << si.name;
        const PdServ::Signal *s = mainTask->addSignal(
                SignalInfo(app_properties.name, &si));
        variableMap[s->path] = s;
    }
    debug() << app_properties.signal_count << "sginals";

    shmem = ::mmap(0, app_properties.rtB_size * app_properties.rtB_count,
            PROT_READ, MAP_PRIVATE, fd, 0);
    if (shmem == MAP_FAILED)
        goto out;

    photoAlbum = reinterpret_cast<const char *>(shmem);

    readPointer = ioctl(fd, RESET_BLOCKIO_RP);

    photoReady = new unsigned int[app_properties.rtB_count];
    std::fill_n(photoReady, app_properties.rtB_count, 0);

    debug() << app_properties.name << app_properties.version;
    startServers();
    debug() << "Servers started";

    do {
        fd_set fds;
        int n;

        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        n = ::select(fd + 1, &fds, 0, 0, 0);
        if (n < 0) {
            // Some error occurred in select()
            goto out;
        }
        else if (!n) {
            // Signal was caught, check whether the children are still there
            //::kill(pid, 0);     // Test existence of pid
            debug() << "caught signal";
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
    ::exit(errno);
}

/////////////////////////////////////////////////////////////////////////////
Main::~Main()
{
    delete parameterBuf;
//     for (size_t i = 0; i < nst; i++)
//         delete task[i];
//     delete[] task;
// 
//     ::munmap(shmem, shmem_len);
}

//                app[i] = new Main(i);
//                std::ostringstream path(device_node);
//                path << i;
//
//                int app_fd = open(path.str().c_str(), O_NONBLOCK, O_RDWR);
//                if (app_fd <= 0) {
//                    debug() << "Could not open" << path.str();
//                    continue;
//                }
//                struct app_properties app_properties;
//                ioctl(app_fd, GET_APP_PROPERTIES, &app_properties);
//
//
//                char name[MAX_APP_NAME_LEN];
//                char version[MAX_APP_VER_LEN];
//                ioctl(app_fd, 

/////////////////////////////////////////////////////////////////////////////
void Main::processPoll(size_t delay_ms,
        const PdServ::Signal * const *s, size_t nelem,
        void * const * pollDest, struct timespec *t) const
{
    const char *data = photoAlbum + readPointer * app_properties.rtB_size;

    for (size_t i = 0; i < nelem; ++i) {
        const Signal *signal = dynamic_cast<const Signal *>(s[i]);
        if (signal) {
            signal->info.read(
                    pollDest[i], data + signal->offset, 0, signal->nelem);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
int Main::gettime(struct timespec *) const
{
    return -1;
}

/////////////////////////////////////////////////////////////////////////////
PdServ::SessionShadow *Main::newSession(PdServ::Session *session) const
{
    return new SessionShadow(session, mainTask, readPointer, photoReady,
            photoAlbum, &app_properties);
}

/////////////////////////////////////////////////////////////////////////////
int Main::setParameter(const Parameter *p, size_t startIndex,
        size_t count, const char *data, struct timespec *mtime) const
{
    ost::SemaphoreLock lock(paramMutex);
    struct param_change delta;

    delta.rtP = parameterBuf;
    delta.pos = startIndex * p->width;
    delta.len = count * p->width;                                                        
    delta.count = 0;

    return ioctl(fd, CHANGE_PARAM, &delta);
}

#if 0
int Main::gettime(struct timespec* t) const
{
     return -1; //rttime ? rttime(t) : PdServ::Main::gettime(t);
}

/////////////////////////////////////////////////////////////////////////////
bool Main::processSdo(unsigned int tid, const struct timespec *time) const
{
//     // PollSignal
//     // Receive:
//     //          [0]: PollSignal
//     //          [1]: The number of signals being polled (n)
//     //          [2 .. 2+n]: Index numbers of the signals
//     //
//     // The reply is in the poll area
// 
//     sdo->time = *time;
// 
//     bool finished = true;
//     char *data = sdoData;
//     switch (sdo->type) {
//         case SDOStruct::PollSignal:
//             for (unsigned i = 0; i < sdo->count; i++) {
//                 const Signal *s = sdo->signal[i];
//                 if (s->task->tid == tid) {
//                     sdoTaskTime[task[tid]->tid] = *time;
//                     std::copy((const char *)s->addr,
//                             (const char *)s->addr + s->memSize,
//                             data);
//                 }
//                 else
//                     finished = false;
//                 data += s->memSize;
//             }
//             break;
// 
//         case SDOStruct::WriteParameter:
//             {
//                 const Parameter *p = sdo->parameter;
//                 sdo->errorCode = (*p->trigger)( tid,
//                         p->addr + sdo->startIndex * p->width,
//                         sdoData, sdo->count * p->width, p->priv_data);
//             }
//             break;
//     }
// 
//     return finished;
    return false;
}

/////////////////////////////////////////////////////////////////////////////
void Main::update(int st, const struct timespec *time) const
{
//     if (sdo->reqId != sdo->replyId and processSdo(st, time))
//         sdo->replyId = sdo->reqId;
// 
//     task[st]->txPdo(time);
}

/////////////////////////////////////////////////////////////////////////////
void Main::getValues( const PdServ::Signal * const *s, size_t nelem,
        char *buf, struct timespec *time) const
{
    ost::SemaphoreLock lock(sdoMutex);
//     size_t delay = 10;
//     size_t dataSize = 0;
// 
//     const Signal **sdoSignal = sdo->signal;
//     for (unsigned i = 0; i < nelem; i++) {
//         sdoSignal[i] = dynamic_cast<const Signal*>(s[i]);
//         dataSize += s[i]->memSize;
//         delay = std::max(delay,
//                 (size_t)(sdoSignal[i]->task->sampleTime * 1000 / 2));
//     }
//     sdo->count = nelem;
//     sdo->type = SDOStruct::PollSignal;
//     sdo->reqId++;
// 
//     do {
//         ost::Thread::sleep(delay);
//     } while (sdo->reqId != sdo->replyId);
// 
//     std::copy(sdoData, sdoData + dataSize, buf);
//     while (time and nelem--)
//         *time++ = sdoTaskTime[(*sdoSignal++)->task->tid];
}

/////////////////////////////////////////////////////////////////////////////
Main *Main::newApplication(unsigned int idx)
{
    std::ostringstream path;
    struct app_properties app_properties;

    path << device_node << idx;

    int fd = open(path.str().c_str(), O_NONBLOCK | O_RDONLY);
    if (fd < 0)
        return 0;

    if (ioctl(fd, GET_APP_PROPERTIES, &app_properties))
        return 0;

    try {
        return new Main(fd, app_properties.name, app_properties.version,
                1.0e-9 * app_properties.sample_period);
    }
    catch (int n) {
        return 0;
    }

//     size_t taskMemSize[nst];
// 
//     size_t parameterSize = 0;
//     for (PdServ::Main::Parameters::iterator it = parameters.begin();
//             it != parameters.end(); it++) {
//         const Parameter *p = dynamic_cast<const Parameter*>(*it);
//         parameterSize += p->memSize;
//     }
// 
//     for (unsigned int i = 0; i < nst; i++) {
//         taskMemSize[i] = ptr_align(task[i]->getShmemSpace(bufferTime));
//         shmem_len += taskMemSize[i];
//     }
// 
//     size_t signalSize = 0;
//     for (PdServ::Main::Signals::const_iterator it = signals.begin();
//             it != signals.end(); it++) {
//         signalSize += (*it)->memSize;
//     }
// 
//     size_t sdoCount = std::max(signals.size(), getParameters().size());
// 
//     shmem_len += sizeof(*sdo) * sdoCount
//         + sizeof(*sdoTaskTime) * nst
//         + std::max(parameterSize, signalSize);
// 
//     shmem = ::mmap(0, shmem_len, PROT_READ | PROT_WRITE,
//             MAP_SHARED | MAP_ANON, -1, 0);
//     if (MAP_FAILED == shmem) {
//         // log(LOGCRIT, "could not mmap
//         // err << "mmap(): " << strerror(errno);
//         ::perror("mmap()");
//         return -errno;
//     }
//     ::memset(shmem, 0, shmem_len);
// 
//     sdo           = ptr_align<SDOStruct>(shmem);
//     sdoTaskTime   = ptr_align<struct timespec>(sdo->signal + sdoCount);
//     sdoData       = ptr_align<char>(sdoTaskTime + nst);
// 
//     char* buf = ptr_align<char>(sdoData + std::max(parameterSize, signalSize));
//     for (unsigned int i = 0; i < nst; i++) {
//         task[i]->prepare(buf, buf + taskMemSize[i]);
//         buf += taskMemSize[i];
//     }
// 
//     pid = ::fork();
//     if (pid < 0) {
//         // Some error occurred
//         ::perror("fork()");
//         return pid;
//     }
//     else if (pid) {
//         // Parent here; go back to the caller
//         for (unsigned int i = 0; i < nst; i++)
//             task[i]->rt_init();
//         return 0;
//     }
// 
//     for (unsigned int i = 0; i < nst; i++)
//         task[i]->nrt_init();
// 
//     pid = getpid();
// 
//     return startProtocols();
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
PdServ::Receiver *Main::newReceiver(unsigned int tid)
{
//     return task[tid]->newReceiver();
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
void Main::subscribe(PdServ::Session *session,
        const PdServ::Signal * const *s, size_t n) const
{
//     size_t count = 0;
//     for (Task ** t = task; t != task + nst; t++) {
//         count += (*t)->subscribe(session, s, n);
//         if (count == n)
//             return;
//     }
}

/////////////////////////////////////////////////////////////////////////////
void Main::unsubscribe(PdServ::Session *session,
        const PdServ::Signal * const *s, size_t n) const
{
//     for (Task ** t = task; t != task + nst; t++)
//         (*t)->unsubscribe(session, s, n);
}
#endif
