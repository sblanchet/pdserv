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

#include <unistd.h>     // exit()
#include <cerrno>       // errno
#include <cstdio>       // perror()
#include <algorithm>    // std::min()
#include <sys/mman.h>   // mmap(), munmap()
#include <sys/types.h>  // kill()
#include <signal.h>     // kill()

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

/////////////////////////////////////////////////////////////////////////////
struct SDOStruct {
    const Parameter *parameter;
    unsigned int offset;
    unsigned int count;
    int rv;
    struct timespec time;
};

struct SessionData {
    struct EventData* eventReadPointer;
};

/////////////////////////////////////////////////////////////////////////////
const double Main::bufferTime = 2.0;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Main::Main( const char *name, const char *version,
        int (*gettime)(struct timespec*)):
    PdServ::Main(name, version),
    mutex(1), eventMutex(1), sdoMutex(1),
    rttime(gettime ? gettime : &PdServ::Main::localtime)
{
    shmem_len = 0;
    shmem = 0;
    tSampleMin = ~0U;
}

/////////////////////////////////////////////////////////////////////////////
Main::~Main()
{
    ::kill(pid, SIGTERM);
    ::munmap(shmem, shmem_len);
}

/////////////////////////////////////////////////////////////////////////////
void Main::setConfigFile(const char *file)
{
    configFile = file;
    readConfiguration();
}

/////////////////////////////////////////////////////////////////////////////
int Main::run()
{
    int rv;

    if (configFile.empty())
        readConfiguration();

    // Initialize library
    rv = prefork_init();
    if (rv)
        return rv;

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
        return pid;
    }
    else if (pid) {
        // Parent here. Return to the caller
        return postfork_rt_setup();
    }

    // Only child runs after this point
    pid = getpid();

    // Kill is used to stop all processes
    ::signal(SIGKILL, SIG_DFL);

    //setupDaemon();      // After readConfiguration()!, this changes directory
    setupLogging();
    postfork_nrt_setup();
    startServers();
    rv = runForever();
    stopServers();

    ::exit(rv);
}

/////////////////////////////////////////////////////////////////////////////
int Main::runForever()
{
    int sig, rv;
    sigset_t mask;
    log4cplus::Logger log = log4cplus::Logger::getRoot();

    // Set signal mask
    if (::sigemptyset(&mask)
            or ::sigaddset(&mask, SIGHUP)      // Reread configuration
            or ::sigaddset(&mask, SIGINT)      // Ctrl-C
            or ::sigaddset(&mask, SIGTERM)) {  // standard kill signal
        rv = errno;
        LOG4CPLUS_FATAL(log,
                LOG4CPLUS_TEXT("Failed to set correct signal mask"));
        goto out;
    }


    if (::sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
        rv = errno;
        LOG4CPLUS_FATAL(log,
                LOG4CPLUS_TEXT("Setting sigprocmask() failed: ")
                << LOG4CPLUS_C_STR_TO_TSTRING(::strerror(errno)));
        goto out;
    }

    while (!(errno = sigwait(&mask, &sig))) {
        switch (sig) {
            case SIGHUP:
                LOG4CPLUS_INFO(log,
                        LOG4CPLUS_TEXT("SIGUP received. Reloading configuration"));
                readConfiguration();
                break;

            default:
                LOG4CPLUS_INFO(log,
                        LOG4CPLUS_TEXT("Killed with ")
                        << LOG4CPLUS_TEXT(::strsignal(sig)));
                return 0;
        }
    }

out:
    return rv;
}

/////////////////////////////////////////////////////////////////////////////
int Main::readConfiguration()
{
    const char *env;

    // Load custom configuration file
    if (!configFile.empty()) {
        const char *err = config.load(configFile.c_str());
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
        const char *err = config.load(env);

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
        const char *err = config.load(f);
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

    if (!config) {
        log_debug("No configuration loaded");
    }

    PdServ::Main::setConfig(config);

    return 0;
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
    if (variableSet.find(path) != variableSet.end())
        return 0;

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

    variableSet.insert(path);
    events.push_back(e);

    return e;
}

/////////////////////////////////////////////////////////////////////////////
Parameter* Main::addParameter( const char *path,
        unsigned int mode, const PdServ::DataType& datatype,
        void *addr, size_t n, const size_t *dim)
{
    if (variableSet.find(path) != variableSet.end())
        return 0;

    Parameter *p = new Parameter(this, path, mode, datatype, addr, n, dim);

    variableSet.insert(path);
    parameters.push_back(p);

    return p;
}

/////////////////////////////////////////////////////////////////////////////
Signal* Main::addSignal( Task *task, unsigned int decimation,
        const char *path, const PdServ::DataType& datatype,
        const void *addr, size_t n, const size_t *dim)
{
    if (variableSet.find(path) != variableSet.end())
        return 0;

    Signal *s = task->addSignal(decimation, path, datatype, addr, n, dim);

    variableSet.insert(path);

    return s;
}

/////////////////////////////////////////////////////////////////////////////
bool Main::setEvent(const Event* event,
        size_t element, bool state, const timespec *t) const
{
    ost::SemaphoreLock lock(eventMutex);
    struct EventData *eventData = event->data + element;

    if (eventData->state == state)
        return false;

    eventData->state = state;
    eventData->time = *t;

    **eventDataWp = *eventData;

    eventData = *eventDataWp;
    if (++eventData == eventDataEnd)
        eventData = eventDataStart;
    *eventDataWp = eventData;

    return true;
}

/////////////////////////////////////////////////////////////////////////////
int Main::setParameter(const Parameter *param,
        size_t offset, size_t count, struct timespec *mtime) const
{
    ost::SemaphoreLock lock(sdoMutex);

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

    if (!sdo->rv) {
        mtime->tv_sec = sdo->time.tv_sec;
        mtime->tv_nsec = sdo->time.tv_nsec;
    }

    return sdo->rv;
}

/////////////////////////////////////////////////////////////////////////////
void Main::parameterChanged(const PdServ::Session* session,
        const Parameter *param,
        const char *buf, size_t offset, size_t count) const
{
    PdServ::Main::parameterChanged(session, param, buf, offset, count);
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
    size_t parameterDataOffset[5] = {0, 0, 0, 0, 0};

    // don't need variableSet any more
    variableSet.clear();

    for (PdServ::Main::Parameters::iterator it = parameters.begin();
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
    for (EventList::iterator it = events.begin(); it != events.end(); ++it) {
        eventCount += (*it)->nelem;
    }
    shmem_len +=
        sizeof(*eventData) * eventCount
        + sizeof(*eventDataStart) * 2 * eventCount;

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

    for (PdServ::Main::Parameters::iterator it = parameters.begin();
            it != parameters.end(); it++) {
        const Parameter *p = static_cast<const Parameter*>(*it);
        p->valueBuf =
            sdoData + parameterDataOffset[dataTypeIndex[p->dtype.align()]];
        parameterDataOffset[dataTypeIndex[p->dtype.align()]] += p->memSize;

        std::copy(p->addr, p->addr + p->memSize, p->valueBuf);
    }

    char* buf = ptr_align<char>(sdoData + parameterDataOffset[4]);
    i = 0;
    for (TaskList::iterator it = task.begin(); it != task.end(); ++it) {
        static_cast<Task*>(*it)->prepare(buf, buf + taskMemSize[i]);
        buf += taskMemSize[i++];
    }

    eventData      = ptr_align<struct EventData>(buf);
    eventDataStart = ptr_align<struct EventData>(eventData + eventCount);
    eventDataEnd   = eventDataStart + 2*eventCount;
    eventDataWp    = ptr_align<struct EventData*>(eventDataEnd);
    *eventDataWp   = eventDataStart;

    i = 0;
    for (EventList::iterator it = events.begin(); it != events.end(); ++it) {
        (*it)->data = eventData + i;
        for(size_t j = 0; j < (*it)->nelem; ++j, ++i) {
            eventData[i].event = *it;
            eventData[i].index = j;
        }
    }
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
PdServ::Main::Events Main::getEvents() const
{
    return Events(events.begin(), events.end());
}

/////////////////////////////////////////////////////////////////////////////
void Main::prepare(PdServ::Session *session) const
{
    session->data = new SessionData;
    session->data->eventReadPointer = *eventDataWp;
}

/////////////////////////////////////////////////////////////////////////////
void Main::cleanup(const PdServ::Session *session) const
{
    delete session->data;
}

/////////////////////////////////////////////////////////////////////////////
const PdServ::Event *Main::getNextEvent( const PdServ::Session* session,
        size_t *index, bool *state, struct timespec *t) const
{
    const EventData* eventData = session->data->eventReadPointer;
    if (eventData == *eventDataWp)
        return 0;

    *index = eventData->index;
    *state = eventData->state;
    *t = eventData->time;

    if (++session->data->eventReadPointer == eventDataEnd)
        session->data->eventReadPointer = eventDataStart;

    return eventData->event;
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
                p->valueBuf + s->offset, s->count, p->priv_data);
        s->count = 0;
    }
}
