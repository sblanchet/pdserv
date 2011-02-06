/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdcomserv package.
 *
 *  pdcomserv is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pdcomserv is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pdcomserv. See COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include "config.h"

#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <sys/mman.h>

#include <sys/types.h>
#include <unistd.h>             // fork()
#include <cc++/cmdoptns.h>

#include "Main.h"
#include "Task.h"
#include "Signal.h"
#include "Receiver.h"
#include "Parameter.h"
#include "Pointer.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

static ost::CommandOption *optionList = 0;

ost::CommandOptionNoArg traditional(
        "traditional", "t", "Traditional MSR", false, &optionList
        );

/////////////////////////////////////////////////////////////////////////////
const double Main::bufferTime = 2.0;

/////////////////////////////////////////////////////////////////////////////
Main::Main(int argc, const char *argv[],
        const char *name, const char *version,
        double baserate,
        unsigned int nst, const unsigned int decimation[],
        int (*gettime)(struct timespec*)):
    HRTLab::Main(name, version, baserate, nst),
    task(new Task*[nst]), mutex(1), sdoMutex(1), rttime(gettime)
{
    char *_argv[argc];
    char helpStr[] = STR(PROJECT_NAME) " options";
    for (int i = 0; i < argc; i++) {
        _argv[i] = strdup(argv[i]);
    }

    ost::CommandOptionParse *args = ost::makeCommandOptionParse(
            argc, _argv, helpStr);

    for (size_t i = 0; i < nst; i++)
        task[i] =
            new Task(this, i, baserate * (decimation ? decimation[i] : 1));

    shmem_len = 0;
    shmem = 0;
    traditionalMSR = traditional.numSet;

    delete args;
    for (int i = 0; i < argc; i++) {
        free(_argv[i]);
    }
}

/////////////////////////////////////////////////////////////////////////////
Main::~Main()
{
    for (size_t i = 0; i < nst; i++)
        delete task[i];
    delete[] task;

    ::munmap(shmem, shmem_len);
}

/////////////////////////////////////////////////////////////////////////////
int Main::gettime(struct timespec* t) const
{
    return rttime ? rttime(t) : HRTLab::Main::gettime(t);
}

/////////////////////////////////////////////////////////////////////////////
int Main::setParameter(const Parameter *p, size_t startIndex,
        size_t count, const char *data, struct timespec *mtime) const
{
    ost::SemaphoreLock lock(sdoMutex);

    size_t delay = ~0U;

    for (size_t i = 0; i < nst; i++)
        delay = std::min(delay, (size_t)(task[i]->sampleTime * 1000 / 2));

    // Write the parameters into the shared memory sorted from widest
    // to narrowest data type width. This ensures that the data is
    // allways aligned correctly.
    std::copy(data, data + p->width * count, sdoData);
    sdo->parameter = p;
    sdo->startIndex = startIndex;
    sdo->count = count;
    sdo->type = SDOStruct::WriteParameter;
    sdo->reqId++;

    do {
        ost::Thread::sleep(delay);
    } while (sdo->reqId != sdo->replyId);

    if (!sdo->errorCode)
        *mtime = sdo->time;
    
    return sdo->errorCode;
}

/////////////////////////////////////////////////////////////////////////////
bool Main::processSdo(unsigned int tid, const struct timespec *time) const
{
    // PollSignal
    // Receive:
    //          [0]: PollSignal
    //          [1]: The number of signals being polled (n)
    //          [2 .. 2+n]: Index numbers of the signals
    //
    // The reply is in the poll area

    sdo->time = *time;

    bool finished = true;
    char *data = sdoData;
    switch (sdo->type) {
        case SDOStruct::PollSignal:
            for (unsigned i = 0; i < sdo->count; i++) {
                const Signal *s = sdo->signal[i];
                if (s->task->tid == tid) {
                    sdoTaskTime[task[tid]->tid] = *time;
//                    cout << "copying from " << (void*)s->addr << " to "
//                        << (void*)data
//                        << " for " << s->path << endl;
                    std::copy((const char *)s->addr,
                            (const char *)s->addr + s->memSize,
                            data);
                }
                else
                    finished = false;
                data += s->memSize;
            }
            break;

        case SDOStruct::WriteParameter:
            {
                const Parameter *p = sdo->parameter;
                sdo->errorCode = (*p->trigger)( tid,
                        p->addr + sdo->startIndex * p->width,
                        sdoData, sdo->count * p->width, p->priv_data);
            }
            break;
    }

    return finished;
}

/////////////////////////////////////////////////////////////////////////////
void Main::update(int st, const struct timespec *time) const
{
    if (sdo->reqId != sdo->replyId and processSdo(st, time))
        sdo->replyId = sdo->reqId;

    task[st]->txPdo(time);
}

/////////////////////////////////////////////////////////////////////////////
void Main::getValues( const HRTLab::Signal * const *s, size_t nelem,
        char *buf, struct timespec *time) const
{
    ost::SemaphoreLock lock(sdoMutex);
    size_t delay = 10;
    size_t dataSize = 0;

    const Signal **sdoSignal = sdo->signal;
    for (unsigned i = 0; i < nelem; i++) {
        sdoSignal[i] = dynamic_cast<const Signal*>(s[i]);
        dataSize += s[i]->memSize;
        delay = std::max(delay,
                (size_t)(sdoSignal[i]->task->sampleTime * 1000 / 2));
    }
    sdo->count = nelem;
    sdo->type = SDOStruct::PollSignal;
    sdo->reqId++;

    do {
        ost::Thread::sleep(delay);
    } while (sdo->reqId != sdo->replyId);

    std::copy(sdoData, sdoData + dataSize, buf);
    while (time and nelem--)
        *time++ = sdoTaskTime[(*sdoSignal++)->task->tid];
}

/////////////////////////////////////////////////////////////////////////////
int Main::init()
{
    size_t taskMemSize[nst];

    size_t parameterSize = 0;
    for (HRTLab::Main::Parameters::iterator it = parameters.begin();
            it != parameters.end(); it++) {
        const Parameter *p = dynamic_cast<const Parameter*>(*it);
        parameterSize += p->memSize;
    }

    for (unsigned int i = 0; i < nst; i++) {
        taskMemSize[i] = ptr_align(task[i]->getShmemSpace(bufferTime));
        shmem_len += taskMemSize[i];
    }

    size_t signalSize = 0;
    for (HRTLab::Main::Signals::const_iterator it = signals.begin();
            it != signals.end(); it++) {
        signalSize += (*it)->memSize;
    }

    size_t sdoCount = std::max(signals.size(), getParameters().size());

    shmem_len += sizeof(*sdo) * sdoCount
        + sizeof(*sdoTaskTime) * nst
        + std::max(parameterSize, signalSize);

    shmem = ::mmap(0, shmem_len, PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_ANON, -1, 0);
    if (MAP_FAILED == shmem) {
        // log(LOGCRIT, "could not mmap
        // err << "mmap(): " << strerror(errno);
        ::perror("mmap()");
        return -errno;
    }
    ::memset(shmem, 0, shmem_len);

    sdo           = ptr_align<SDOStruct>(shmem);
    sdoTaskTime   = ptr_align<struct timespec>(sdo->signal + sdoCount);
    sdoData       = ptr_align<char>(sdoTaskTime + nst);

//    cout
//        << " shmem=" << shmem
//        << " sdo=" << sdo
//        << " sdoTaskTime=" << sdoTaskTime
//        << " sdoData=" << (void*)sdoData
//        << endl;

    char* buf = ptr_align<char>(sdoData + std::max(parameterSize, signalSize));
    for (unsigned int i = 0; i < nst; i++) {
        task[i]->prepare(buf, buf + taskMemSize[i]);
        buf += taskMemSize[i];
    }

    pid = ::fork();
    if (pid < 0) {
        // Some error occurred
        ::perror("fork()");
        return pid;
    }
    else if (pid) {
        // Parent here; go back to the caller
        for (unsigned int i = 0; i < nst; i++)
            task[i]->rt_init();
        return 0;
    }

    for (unsigned int i = 0; i < nst; i++)
        task[i]->nrt_init();

    pid = getpid();

    return startProtocols();
}

/////////////////////////////////////////////////////////////////////////////
HRTLab::Receiver *Main::newReceiver(unsigned int tid)
{
    return task[tid]->newReceiver();
}

/////////////////////////////////////////////////////////////////////////////
void Main::subscribe(HRTLab::Session *session,
        const HRTLab::Signal * const *s, size_t n) const
{
    size_t count = 0;
    for (Task ** t = task; t != task + nst; t++) {
        count += (*t)->subscribe(session, s, n);
        if (count == n)
            return;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Main::unsubscribe(HRTLab::Session *session,
        const HRTLab::Signal * const *s, size_t n) const
{
    for (Task ** t = task; t != task + nst; t++)
        (*t)->unsubscribe(session, s, n);
}
