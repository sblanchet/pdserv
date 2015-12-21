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

#include <iostream>
#include <unistd.h>     // exit(), sleep()
#include <cerrno>       // errno
#include <cstdio>       // perror()
#include <algorithm>    // std::min()
#include <sys/mman.h>   // mmap(), munmap()
#include <signal.h>     // signal()
#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

#include "pdserv.h"
#include "Main.h"
#include "Task.h"
#include "Parameter.h"
#include "Signal.h"
#include "Event.h"
#include "Pointer.h"
#include "ShmemDataStructures.h"
#include "../Session.h"
#include "../Config.h"
#include "../Database.h"

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
/////////////////////////////////////////////////////////////////////////////
Main::Main( const char *name, const char *version,
        int (*gettime)(struct timespec*)):
    PdServ::Main(name, version),
    mutex(1), eventMutex(1),
    rttime(gettime ? gettime : &PdServ::Main::localtime)
{
    shmem_len = 0;
    shmem = 0;
    tSampleMin = ~0U;
}

/////////////////////////////////////////////////////////////////////////////
Main::~Main()
{
    while (task.size()) {
        delete task.front();
        task.pop_front();
    }

    while (parameters.size()) {
        delete parameters.front();
        parameters.pop_front();
    }

    ::close(ipc_pipe[1]);
    ::munmap(shmem, shmem_len);
}

/////////////////////////////////////////////////////////////////////////////
void Main::setConfigFile(const char *file)
{
    configFile = file;
}

/////////////////////////////////////////////////////////////////////////////
int Main::run()
{
    int rv;
    time_t persistTimeout;
    fd_set fds;
    struct timeval timeout, now, delay;

    readConfiguration();
    setupLogging();
    persistTimeout = setupPersistent();

    // Initialize library
    rv = prefork_init();
    if (rv)
        return rv;

    // Open a pipe between the two processes. This is used to inform the
    // child that the parent has died
    if (::pipe(ipc_pipe)) {
        rv = errno;
        ::perror("pipe()");
        return rv;
    }

    // Immediately split off a child. The parent returns to the caller so
    // that he can get on with his job.
    //
    // The child continues from here.
    //
    // It is intentional that the child has the same process group as
    // the parent so that it gets all the signals too.
    pid = ::fork();
    if (pid < 0) {
        // Some error occurred
        ::perror("fork()");
        return errno;
    }
    else if (pid) {
        // Parent here. Return to the caller

        // Close read end of pipe
        ::close(ipc_pipe[0]);

        // Send a signal to the child that parent is running
        ::write(ipc_pipe[1], &pid, 1);

        return postfork_rt_setup();
    }

    // Only child runs after this point
    pid = getpid();

    // Close write end of pipe
    ::close(ipc_pipe[1]);

    // Wait till main thread is initialized
    if (::read(ipc_pipe[0], &rv, 1) != 1)
        exit(errno);

    // Ignore common terminating signals
    ::signal(SIGINT, SIG_IGN);
    ::signal(SIGTERM, SIG_IGN);

    postfork_nrt_setup();

    startServers();

    ::gettimeofday(&timeout, 0);
    timeout.tv_sec += persistTimeout;

    FD_ZERO(&fds);

    // Stay in this loop until real-time thread exits, in which case
    // ipc_pipe[0] becomes readable
    struct ::EventData* eventData = *eventDataWp;
    do {
        for (TaskList::iterator it = task.begin();
                it != task.end(); ++it)
            static_cast<Task*>(*it)->nrt_update();

        if (persistTimeout) {
            if (::gettimeofday(&now, 0)) {
                rv = errno;
                break;
            }

            if ( now.tv_sec >= timeout.tv_sec) {
                timeout.tv_sec += persistTimeout;
                savePersistent();
            }
        }

        while (eventData != *eventDataWp) {
            newEvent(eventData->event, eventData->index,
                    eventData->state, &eventData->time);
            if (++eventData == eventDataEnd)
                eventData = eventDataStart;
        }

        FD_SET(ipc_pipe[0], &fds);
        delay.tv_sec = 1;
        rv = ::select(ipc_pipe[0] + 1, &fds, 0, 0, &delay);
    } while (!rv);

    // Ignore rv if ipc_pipe[0] is readable
    if (rv == 1)
        rv = 0;

    stopServers();

    savePersistent();

    ::exit(rv);
}

/////////////////////////////////////////////////////////////////////////////
int Main::readConfiguration()
{
    const char *env;
    const char *err = 0;

    // Load custom configuration file
    if (!configFile.empty()) {
        err = m_config.load(configFile.c_str());
        if (err)
            std::cout
                << "Error loading configuration file "
                << configFile << " specified on command line: " << err << std::endl;
        else {
            log_debug("Loaded specified configuration file %s",
                    configFile.c_str());
        }
    }
    else if ((env = ::getenv("PDSERV_CONFIG")) and ::strlen(env)) {
        // Try to load environment configuration file
        err = m_config.load(env);

        if (err)
            std::cout << "Error loading config file " << env
                << " specified in environment variable PDSERV_CONFIG: "
                << err << std::endl;
        else {
            log_debug("Loaded ENV config %s", env);
        }
    }
    else {
        // Try to load default configuration file
        const char *f = QUOTE(SYSCONFDIR) "/pdserv.conf";
        err = m_config.load(f);
        if (err) {
            std::cout << "Error loading default config file " << f << ": "
                << (::access(f, R_OK)
                        ? ": File is not readable"
                        : err)
                << std::endl;
        }
        else {
            log_debug("Loaded default configuration file %s", f);
        }
    }

    if (!m_config or err) {
        log_debug("No configuration loaded");
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
PdServ::Config Main::config(const char* key) const
{
    return m_config[key];
}

/////////////////////////////////////////////////////////////////////////////
Task* Main::addTask(double sampleTime, const char *name)
{
    Task *t = new Task(this, sampleTime, name);
    task.push_back(t);

    tSampleMin =
        std::min(tSampleMin, static_cast<size_t>(sampleTime * 1000 + 0.5));
    if (!tSampleMin)
        tSampleMin = 1U;

    return t;
}

/////////////////////////////////////////////////////////////////////////////
int Main::gettime(struct timespec* t) const
{
    return rttime(t);
}

/////////////////////////////////////////////////////////////////////////////
Event* Main::addEvent (
        const char *path, int prio, size_t nelem, const char **messages)
{
    PdServ::Event::Priority eventPrio;
    switch (prio) {
        case 1: eventPrio = PdServ::Event::Alert;       break;
        case 2: eventPrio = PdServ::Event::Critical;    break;
        case 3: eventPrio = PdServ::Event::Error;       break;
        case 4: eventPrio = PdServ::Event::Warning;     break;
        case 5: eventPrio = PdServ::Event::Notice;      break;
        case 6: eventPrio = PdServ::Event::Info;        break;
        default:
            eventPrio = prio <= 0
                ? PdServ::Event::Emergency
                : PdServ::Event::Debug;
    }
    Event *e = new Event(this, path, eventPrio, nelem, messages);

    events.push_back(e);

    return e;
}

/////////////////////////////////////////////////////////////////////////////
Parameter* Main::addParameter( const char *path,
        unsigned int mode, const PdServ::DataType& datatype,
        void *addr, size_t n, const size_t *dim)
{
    Parameter *p = new Parameter(this, addr, path, mode, datatype, n, dim);

    parameters.push_back(p);

    return p;
}

/////////////////////////////////////////////////////////////////////////////
Signal* Main::addSignal( Task *task, unsigned int decimation,
        const char *path, const PdServ::DataType& datatype,
        const void *addr, size_t n, const size_t *dim)
{
    return task->addSignal(decimation, path, datatype, addr, n, dim);
}

/////////////////////////////////////////////////////////////////////////////
void Main::setEvent(Event* event,
        size_t element, bool state, const timespec *time) const
{
    ost::SemaphoreLock lock(eventMutex);

    struct ::EventData *eventData = *eventDataWp;
    eventData->event = event;
    eventData->index = element;
    eventData->state = state;
    eventData->time  = *time;

    if (++eventData == eventDataEnd)
        eventData = eventDataStart;
    *eventDataWp = eventData;
}

/////////////////////////////////////////////////////////////////////////////
int Main::setParameter(const PdServ::ProcessParameter *p,
        size_t offset, size_t count) const
{
    const Parameter* param = static_cast<const Parameter*>(p);

    // Write the parameters into the shared memory sorted from widest
    // to narrowest data type size. This ensures that the data is
    // always aligned correctly.
    sdo->parameter = param;
    sdo->offset = offset;

    // Now write the length to trigger the action
    sdo->count = count;

    do {
        ost::Thread::sleep(tSampleMin);
    } while (sdo->count);

    if (!sdo->rv)
        // Save time of update
        param->mtime = sdo->time;

    return sdo->rv;
}

/////////////////////////////////////////////////////////////////////////////
void Main::initializeParameter(const PdServ::Parameter* p,
        const char* data, const struct timespec* mtime,
        const PdServ::Signal* s)
{
    if (data) {
        log_debug("Restoring %s", p->path.c_str());
        const Parameter *parameter = static_cast<const Parameter*>(p);
        std::copy(data, data + parameter->memSize, parameter->addr);
        parameter->mtime = *mtime;
    }

    if (s) {
        const Signal* signal = static_cast<const Signal*>(s);
        signal->task->makePersistent(signal);
    }
}

/////////////////////////////////////////////////////////////////////////////
bool Main::getPersistentSignalValue(const PdServ::Signal *s,
        char* buf, struct timespec* time)
{
    const struct timespec* t;
    bool rv = static_cast<const Signal*>(s)->task->getPersistentValue(
            s, buf, &t);

    if (rv and time)
        *time = *t;

    return rv;
}

/////////////////////////////////////////////////////////////////////////////
void Main::processPoll(unsigned int delay_ms,
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
// Orangization of shared memory:
//      struct SDOStruct        sdo
//      char                    sdoData (binary data of all parameters)
//      char                    pdoData
//      struct EventData        eventDataStart
//
int Main::prefork_init()
{
    size_t numTasks = task.size();
    size_t taskMemSize[numTasks];
    size_t i, eventCount;

    // The following two variables are used to organize parameters according
    // to the size of their elements so that their data type alignment is
    // correct.
    //
    // dataTypeIndex[] maps the data type to the index in parameterDataOffset,
    // e.g. a parameter with data type double (sizeof() = 8) will then go into
    // container parameterDataOffset[dataTypeIndex[8]]
    //
    // parameterDataOffset[] holds the start index of a data types with
    // 8, 4, 2 and 1 bytes alignment
    const size_t dataTypeIndex[PdServ::DataType::maxWidth+1] = {
        3 /*0*/, 3 /*1*/, 2 /*2*/, 3 /*3*/,
        1 /*4*/, 3 /*5*/, 3 /*6*/, 3 /*7*/, 0 /*8*/
    };
    size_t parameterDataOffset[5] = {0, 0, 0, 0, 0};    // need one extra!

    for (ParameterList::iterator it = parameters.begin();
            it != parameters.end(); it++) {
        const Parameter *p = static_cast<const Parameter*>(*it);

        // Push the next smaller data type forward by the parameter's
        // memory requirement
        parameterDataOffset[dataTypeIndex[p->dtype.align()] + 1] += p->memSize;
    }

    // Accumulate the offsets so that they follow each other in the shared
    // data space. This also has the effect, that the value of
    // parameterDataOffset[4] is the total memory requirement of all
    // parameters
    for (i = 1; i < 5; ++i)
        parameterDataOffset[i] += parameterDataOffset[i-1];

    // Extend shared memory size with the parameter memory requirement
    // and as many sdo's for every parameter.
    shmem_len += sizeof(*sdo) * (parameters.size() + 1)
        + parameterDataOffset[4];

    // Now check how much memory is required for events
    eventCount = 0;
    for (EventList::iterator it = events.begin(); it != events.end(); ++it)
        eventCount += (*it)->nelem;

    // Increase shared memory by the number of events as well as
    // enough capacity to store 10 event changes
    shmem_len += sizeof(*eventDataStart) * 10 * eventCount;

    shmem_len += sizeof(*eventDataWp);  // Memory location for write pointer

    // Find out the memory requirement for the tasks to pipe their variables
    // out of the real time environment
    i = 0;
    for (TaskList::const_iterator it = task.begin();
            it != task.end(); ++it) {
        taskMemSize[i] = ptr_align(
                static_cast<const Task*>(*it)->getShmemSpace(bufferTime));
        //log_debug("Task %i %f shmlen=%zu", i, bufferTime, taskMemSize[i]);
        shmem_len += taskMemSize[i++];
    }

    // Fudge factor. Every ptr_align can silently increase the pointer in
    // shmem by sizeof(unsigned long).
    // At the moment there are roughly 6 ptr_align's, take 10 to make sure!
    shmem_len += 10*sizeof(unsigned long);

    shmem = ::mmap(0, shmem_len, PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_ANON, -1, 0);
    if (MAP_FAILED == shmem) {
        // log(LOGCRIT, "could not mmap
        // err << "mmap(): " << strerror(errno);
        ::perror("mmap()");
        return errno;
    }
    ::memset(shmem, 0, shmem_len);

    sdo     = ptr_align<struct SDOStruct>(shmem);
    sdoData = ptr_align<char>(ptr_align<double>(sdo + parameters.size() + 1));

    for (ParameterList::iterator it = parameters.begin();
            it != parameters.end(); it++) {
        Parameter *p = *it;
        p->shmAddr =
            sdoData + parameterDataOffset[dataTypeIndex[p->dtype.align()]];
        parameterDataOffset[dataTypeIndex[p->dtype.align()]] += p->memSize;

        std::copy(p->addr, p->addr + p->memSize, p->shmAddr);
    }

    char* buf = ptr_align<char>(sdoData + parameterDataOffset[4]);
    i = 0;
    for (TaskList::iterator it = task.begin(); it != task.end(); ++it) {
        static_cast<Task*>(*it)->prepare(buf, buf + taskMemSize[i]);
        buf += taskMemSize[i++];
    }

    eventDataWp    = ptr_align<struct ::EventData*>(buf);
    eventDataStart = ptr_align<struct ::EventData>(eventDataWp + 1);
    eventDataEnd   = ptr_align<struct ::EventData>((char*)shmem + shmem_len) - 1;
    *eventDataWp   = eventDataStart;

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
int Main::postfork_rt_setup()
{
    // Parent here; go back to the caller
    for (TaskList::iterator it = task.begin();
            it != task.end(); ++it)
        static_cast<Task*>(*it)->rt_init();
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
int Main::postfork_nrt_setup()
{
    // Parent here; go back to the caller
    for (TaskList::iterator it = task.begin();
            it != task.end(); ++it)
        static_cast<Task*>(*it)->nrt_init();
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
std::list<const PdServ::Parameter*> Main::getParameters() const
{
    return std::list<const PdServ::Parameter*>(
            parameters.begin(), parameters.end());
}

/////////////////////////////////////////////////////////////////////////////
std::list<const PdServ::Event*> Main::getEvents() const
{
    return std::list<const PdServ::Event*>(events.begin(), events.end());
}

/////////////////////////////////////////////////////////////////////////////
std::list<const PdServ::Task*> Main::getTasks() const
{
    return std::list<const PdServ::Task*>(task.begin(), task.end());
}

/////////////////////////////////////////////////////////////////////////////
void Main::prepare(PdServ::Session* /*session*/) const
{
}

/////////////////////////////////////////////////////////////////////////////
void Main::cleanup(const PdServ::Session* /*session*/) const
{
}

/////////////////////////////////////////////////////////////////////////////
void Main::getParameters(Task *task, const struct timespec *t) const
{
    for (struct SDOStruct *s = sdo; s->count; s++) {
        const Parameter *p = s->parameter;
        const PdServ::Variable *v = p;
        s->time.tv_sec = t->tv_sec;
        s->time.tv_nsec = t->tv_nsec;
        s->rv = p->trigger(reinterpret_cast<struct pdtask *>(task),
                reinterpret_cast<const struct pdvariable *>(v),
                p->addr + s->offset,
                p->shmAddr + s->offset, s->count, p->priv_data);
        s->count = 0;
    }
}
