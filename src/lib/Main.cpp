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

#include "../Debug.h"

#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <algorithm>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>             // fork(), getpid(), chdir, sysconf
#include <signal.h>             // signal()
#include <fcntl.h>              // open()
#include <limits.h>             // _POSIX_OPEN_MAX

#include "config.h"
#include "Main.h"
#include "Task.h"
#include "Signal.h"
#include "Parameter.h"
#include "Pointer.h"
#include "SessionShadow.h"
#include "pdserv/pdserv.h"

/////////////////////////////////////////////////////////////////////////////
struct SDOStruct {
    const Parameter *parameter;
    unsigned int offset;
    unsigned int count;
    int rv;
    struct timespec time;
};

/////////////////////////////////////////////////////////////////////////////
const double Main::bufferTime = 2.0;

/////////////////////////////////////////////////////////////////////////////
Main::Main( const char *name, const char *version,
        int (*gettime)(struct timespec*)):
    mutex(1), sdoMutex(1),
    rttime(gettime ? gettime : &PdServ::Main::localtime)
{
    this->name = name;
    this->version = version;

    shmem_len = 0;
    shmem = 0;
}

/////////////////////////////////////////////////////////////////////////////
Main::~Main()
{
    ::kill(pid, SIGHUP);

    ::munmap(shmem, shmem_len);
}

/////////////////////////////////////////////////////////////////////////////
int Main::configFile(const std::string& file)
{
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
Task* Main::addTask(double sampleTime, const char *name)
{
    Task *t = new Task(this, sampleTime, name);
    task.push_back(t);
    return t;
}

/////////////////////////////////////////////////////////////////////////////
Task *Main::getTask(size_t index) const
{
    return static_cast<Task*>(task[index]);
}

/////////////////////////////////////////////////////////////////////////////
int Main::gettime(struct timespec* t) const
{
    return rttime(t);
}

/////////////////////////////////////////////////////////////////////////////
Parameter* Main::addParameter( const char *path,
        unsigned int mode, PdServ::Variable::Datatype datatype,
        void *addr, size_t n, const unsigned int *dim)
{
    if (variableMap.find(path) != variableMap.end())
        return 0;

    Parameter *p = new Parameter(this, path, mode, datatype, addr, n, dim);

    variableMap[path] = p;
    parameters.push_back(p);

    return p;
}

/////////////////////////////////////////////////////////////////////////////
Signal* Main::addSignal( Task *task, unsigned int decimation,
        const char *path, PdServ::Variable::Datatype datatype,
        const void *addr, size_t n, const unsigned int *dim)
{
    if (variableMap.find(path) != variableMap.end())
        return 0;

    Signal *s = task->addSignal(decimation, path, datatype, addr, n, dim);

    variableMap[path] = s;

    return s;
}

/////////////////////////////////////////////////////////////////////////////
int Main::setParameter(const Parameter *p, size_t startIndex,
        size_t count, const char *data, struct timespec *mtime) const
{
    ost::SemaphoreLock lock(sdoMutex);

    size_t delay = 100; // ms FIXME

    // Write the parameters into the shared memory sorted from widest
    // to narrowest data type width. This ensures that the data is
    // allways aligned correctly.
    sdo->parameter = p;
    sdo->offset = p->width * startIndex;
    size_t n = p->width * count;
    std::copy(data, data + n, p->valueBuf + sdo->offset);

    // Now write the length to trigger the action
    sdo->count = n;

    do {
        ost::Thread::sleep(delay);
    } while (sdo->count);

    if (!sdo->rv)
        *mtime = sdo->time;
    
    return sdo->rv;
}

/////////////////////////////////////////////////////////////////////////////
void Main::processPoll(size_t delay_ms,
        const PdServ::Signal * const *s, size_t nelem,
        void * const *pollDest, struct timespec *t) const
{
    bool fin[task.size()];
    bool finished;
    std::fill_n(fin, 4, false);
    do {
        finished = true;
        int i = 0;
        for (TaskList::const_iterator it = task.begin();
                it != task.end(); ++it, ++i) {
            const Task *task = static_cast<const Task*>(*it);
            fin[i] = fin[i] or task->pollFinished( s, nelem, pollDest, t);
            finished = finished and fin[i];
        }
        ost::Thread::sleep(delay_ms);
    } while (!finished);
}

/////////////////////////////////////////////////////////////////////////////
int Main::run()
{
    size_t numTasks = task.size();
    size_t taskMemSize[numTasks];
    size_t i;
    const size_t dataTypeIndex[PdServ::Variable::maxWidth+1] = {
        3 /*0*/, 3 /*1*/, 2 /*2*/, 3 /*3*/,
        1 /*4*/, 3 /*5*/, 3 /*6*/, 3 /*7*/, 0 /*8*/
    };

    size_t parameterOffset[5];
    std::fill_n(parameterOffset, 5, 0);

    size_t parameterSize = 0;
    for (PdServ::Main::Parameters::iterator it = parameters.begin();
            it != parameters.end(); it++) {
        const Parameter *p = dynamic_cast<const Parameter*>(*it);
        parameterOffset[dataTypeIndex[p->width] + 1] += p->memSize;
        parameterSize += p->memSize;
    }

    i = 0;
    for (TaskList::const_iterator it = task.begin();
            it != task.end(); ++it) {
        taskMemSize[i] = ptr_align(
                static_cast<const Task*>(*it)->getShmemSpace(bufferTime));
        shmem_len += taskMemSize[i++];
    }

    shmem_len += sizeof(*sdo) * (parameters.size() + 1)
//        + sizeof(*sdoTaskTime) * numTasks
        + parameterSize;

    shmem = ::mmap(0, shmem_len, PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_ANON, -1, 0);
    if (MAP_FAILED == shmem) {
        // log(LOGCRIT, "could not mmap
        // err << "mmap(): " << strerror(errno);
        ::perror("mmap()");
        return -errno;
    }
    ::memset(shmem, 0, shmem_len);

    sdo     = ptr_align<struct SDOStruct>(shmem);
    sdoData = ptr_align<char>(ptr_align<double>(sdo + parameters.size() + 1));

    debug() << "shmemlen=" << shmem_len
        << " shmem=" << shmem
        << " sdo=" << sdo
//        << " sdoTaskTime=" << sdoTaskTime
        << " sdoData=" << (void*)sdoData;

    for (PdServ::Main::Parameters::iterator it = parameters.begin();
            it != parameters.end(); it++) {
        const Parameter *p = dynamic_cast<const Parameter*>(*it);
        p->valueBuf = sdoData + parameterOffset[dataTypeIndex[p->width]];
        parameterOffset[dataTypeIndex[p->width]] += p->memSize;

        std::copy(p->addr, p->addr + p->memSize, p->valueBuf);
    }

    char* buf = ptr_align<char>(sdoData + parameterSize);
    i = 0;
    for (TaskList::iterator it = task.begin();
            it != task.end(); ++it) {
        static_cast<Task*>(*it)->prepare(buf, buf + taskMemSize[i]);
        buf += taskMemSize[i];
    }

    return daemonize();
}

/////////////////////////////////////////////////////////////////////////////
int Main::daemonize()
{
    pid = ::fork();
    if (pid < 0) {
        // Some error occurred
        ::perror("fork()");
        return pid;
    }
    else if (pid) {
        // Parent here; go back to the caller
        for (TaskList::iterator it = task.begin();
                it != task.end(); ++it)
            static_cast<Task*>(*it)->rt_init();
        return 0;
    }

    // Only child runs after this point
    pid = getpid();

    // Go to root
#ifndef DEBUG
    ::chdir("/");
    ::umask(000);
#endif

    // Reset signal handlers
    ::signal(SIGHUP,  SIG_DFL);
    ::signal(SIGINT,  SIG_DFL);
    ::signal(SIGTERM, SIG_DFL);

    // close all file handles
    // sysconf() usually returns one more than max file handle
    long int fd_max = ::sysconf(_SC_OPEN_MAX);
    if (fd_max < _POSIX_OPEN_MAX)
        fd_max = _POSIX_OPEN_MAX;
    else if (fd_max > 100000)
        fd_max = 512;   // no rediculous values please
    while (fd_max--)
        ::close(fd_max);

    // Reopen STDIN, STDOUT and STDERR
    int fd = -1;
#ifdef DEBUG
    fd = ::open("/dev/tty", O_RDWR);
#endif
    if (fd < 0)
        fd = ::open("/dev/null", O_RDWR);
    if (fd == STDIN_FILENO) {
        dup2(STDIN_FILENO, STDOUT_FILENO);
        dup2(STDIN_FILENO, STDERR_FILENO);

        fcntl( STDIN_FILENO, F_SETFL, O_RDONLY);
        fcntl(STDOUT_FILENO, F_SETFL, O_WRONLY);
        fcntl(STDERR_FILENO, F_SETFL, O_WRONLY);
    }

    // Initialize non-real time tasks
    for (TaskList::iterator it = task.begin(); it != task.end(); ++it)
        static_cast<Task*>(*it)->nrt_init();

    PdServ::Main::startServers();

    // Hang here forever
    while (true) {
        pause();
    }
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
void Main::getParameters(Task *task, const struct timespec *t) const
{
    for (struct SDOStruct *s = sdo; s->count; s++) {
        const Parameter *p = s->parameter;
        const PdServ::Variable *v = p;
        s->time = *t;
        s->rv = p->trigger(reinterpret_cast<struct pdtask *>(task),
                reinterpret_cast<const struct pdvariable *>(v),
                p->addr + s->offset,
                p->valueBuf + s->offset, s->count, p->priv_data);
        s->count = 0;
    }
}

/////////////////////////////////////////////////////////////////////////////
PdServ::SessionShadow *Main::newSession(PdServ::Session *session) const
{
    return new SessionShadow(this, session);
}
