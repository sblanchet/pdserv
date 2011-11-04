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

#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <sys/mman.h>

#include <sys/types.h>
#include <unistd.h>             // fork()
//#include <cc++/cmdoptns.h>

#include "Main.h"
#include "Task.h"
#include "Signal.h"
#include "Parameter.h"
#include "Pointer.h"
#include "pdserv/pdserv.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

// static ost::CommandOption *optionList = 0;
// 
// ost::CommandOptionNoArg traditional(
//         "traditional", "t", "Traditional MSR", false, &optionList
//         );

struct SDOStruct {
    const Parameter *parameter;
    unsigned int startIndex;
    unsigned int count;
};

/////////////////////////////////////////////////////////////////////////////
const double Main::bufferTime = 2.0;

/////////////////////////////////////////////////////////////////////////////
Main::Main(int argc, const char **argv,
        const char *name, const char *version,
        int (*gettime)(struct timespec*)):
    PdServ::Main(argc, argv, name, version), mutex(1), // sdoMutex(1),
    rttime(gettime ? gettime : &PdServ::Main::localtime)
{
    shmem_len = 0;
    shmem = 0;
}

/////////////////////////////////////////////////////////////////////////////
Main::~Main()
{
    ::munmap(shmem, shmem_len);
}

/////////////////////////////////////////////////////////////////////////////
Task* Main::addTask(double sampleTime, const char *name)
{
    Task *t = new Task(this, sampleTime, name);
    taskList.push_back(t);
    return t;
}

/////////////////////////////////////////////////////////////////////////////
int Main::gettime(struct timespec* t) const
{
    return rttime(t);
}

/////////////////////////////////////////////////////////////////////////////
Parameter* Main::addParameter( const char *path,
        unsigned int mode, enum si_datatype_t datatype,
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
        const char *path, enum si_datatype_t datatype,
        const void *addr, size_t n, const unsigned int *dim)
{
    if (variableMap.find(path) != variableMap.end())
        return 0;

    Signal *s = task->addSignal(decimation, path, datatype, addr, n, dim);

    variableMap[path] = s;
    signals.push_back(s);

    return s;
}

// /////////////////////////////////////////////////////////////////////////////
// int Main::setParameter(const Parameter *p, size_t startIndex,
//         size_t count, const char *data, struct timespec *mtime) const
// {
//     ost::SemaphoreLock lock(sdoMutex);
// 
//     size_t delay = 100; // ms FIXME
// 
//     // Write the parameters into the shared memory sorted from widest
//     // to narrowest data type width. This ensures that the data is
//     // allways aligned correctly.
//     std::copy(data, data + p->width * count, sdoData);
//     sdo->parameter = p;
//     sdo->startIndex = startIndex;
//     sdo->count = count;
//     sdo->type = SDOStruct::WriteParameter;
//     sdo->reqId++;
// 
//     do {
//         ost::Thread::sleep(delay);
//     } while (sdo->reqId != sdo->replyId);
// 
//     if (!sdo->errorCode)
//         *mtime = sdo->time;
//     
//     return sdo->errorCode;
// }

/////////////////////////////////////////////////////////////////////////////
void Main::processPoll(size_t delay_ms) const
{
    bool fin[taskList.size()];
    bool finished;
    std::fill_n(fin, 4, false);
    do {
        finished = true;
        int i = 0;
//        cout << "wait ";
        for (TaskList::const_iterator it = taskList.begin();
                it != taskList.end(); ++it, ++i) {
            fin[i] = fin[i] or (*it)->pollFinished();
            finished = finished and fin[i];
//            cout << fin[i] << ' ';
        }
//        cout << "sleep" << endl;
        ost::Thread::sleep(delay_ms);
    } while (!finished);
}

/////////////////////////////////////////////////////////////////////////////
int Main::run()
{
    size_t numTasks = taskList.size();
    size_t taskMemSize[numTasks];
    size_t i;

    size_t parameterSize = 0;
    for (PdServ::Main::Parameters::iterator it = parameters.begin();
            it != parameters.end(); it++) {
        const Parameter *p = dynamic_cast<const Parameter*>(*it);
        parameterSize += p->memSize;
    }

    i = 0;
    for (TaskList::const_iterator it = taskList.begin();
            it != taskList.end(); ++it) {
        taskMemSize[i] = ptr_align(
                static_cast<const Task*>(*it)->getShmemSpace(bufferTime));
        shmem_len += taskMemSize[i++];
    }

    shmem_len += sizeof(*sdo) * parameters.size()
        + sizeof(*sdoTaskTime) * numTasks
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

//    sdo           = ptr_align<SDOStruct>(shmem);
//    sdoTaskTime   = ptr_align<struct timespec>(sdo->signal + sdoCount);
    sdoData       = ptr_align<char>(shmem); //sdoTaskTime + numTasks);

   cout << "shmemlen=" << shmem_len
       << " shmem=" << shmem
       << " sdo=" << sdo
       << " sdoTaskTime=" << sdoTaskTime
       << " sdoData=" << (void*)sdoData
       << endl;

    char* buf = ptr_align<char>(sdoData + parameterSize);
    i = 0;
    for (TaskList::iterator it = taskList.begin();
            it != taskList.end(); ++it) {
        static_cast<Task*>(*it)->prepare(buf, buf + taskMemSize[i]);
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
        for (TaskList::iterator it = taskList.begin();
                it != taskList.end(); ++it)
            static_cast<Task*>(*it)->rt_init();
        return 0;
    }

    for (TaskList::iterator it = taskList.begin(); it != taskList.end(); ++it)
        static_cast<Task*>(*it)->nrt_init();

    pid = getpid();

    return PdServ::Main::run();
}
