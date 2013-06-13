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
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <sstream>
#include <algorithm>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>             // fork(), getpid(), chdir, sysconf
#include <signal.h>             // signal()
#include <fcntl.h>              // open()
#include <limits.h>             // _POSIX_OPEN_MAX

#include <log4cplus/logger.h>
#include <log4cplus/syslogappender.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/streams.h>
#include <log4cplus/configurator.h>
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
#include "../DataType.h"
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
Main::Main( const char *name, const char *version,
        int (*gettime)(struct timespec*)):
    PdServ::Main(name, version),
    mutex(1), eventMutex(1), sdoMutex(1),
    rttime(gettime ? gettime : &PdServ::Main::localtime)
{
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
void Main::setConfigFile(const char *file)
{
    configFile = file;
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
int Main::setParameter(const Parameter *p, size_t offset,
        size_t count, const char *data, struct timespec *mtime) const
{
    ost::SemaphoreLock lock(sdoMutex);

    size_t delay = 100; // ms FIXME

    // Write the parameters into the shared memory sorted from widest
    // to narrowest data type size. This ensures that the data is
    // always aligned correctly.
    sdo->parameter = p;
    sdo->offset = offset;
    std::copy(data, data + count, p->valueBuf + sdo->offset);

    // Now write the length to trigger the action
    sdo->count = count;

    do {
        ost::Thread::sleep(delay);
    } while (sdo->count);

    if (!sdo->rv)
        *mtime = sdo->time;

    return sdo->rv;
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
int Main::run()
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

    for (PdServ::Main::ProcessParameters::iterator it = parameters.begin();
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

//     cerr_debug() << "shmemlen=" << shmem_len
//         << " shmem=" << shmem
//         << " sdo=" << sdo
// //        << " sdoTaskTime=" << sdoTaskTime
//         << " sdoData=" << (void*)sdoData;

    for (PdServ::Main::ProcessParameters::iterator it = parameters.begin();
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
//    log_debug("%zu %p %p %zu", eventCount, eventDataWp+1, (char*)shmem + shmem_len, shmem_len);

    return daemonize();
}

/////////////////////////////////////////////////////////////////////////////
void Main::consoleLogging()
{
    log4cplus::BasicConfigurator::doConfigure();
//    log4cplus::SharedAppenderPtr cerr(new log4cplus::ConsoleAppender(true));
//    cerr->setLayout(
//        std::auto_ptr<log4cplus::Layout>(new log4cplus::PatternLayout(
//                LOG4CPLUS_TEXT("%D %p %c %x: %m"))));
//
//    log4cplus::Logger::getRoot().addAppender(cerr);
}

/////////////////////////////////////////////////////////////////////////////
void Main::syslogLogging()
{
    log4cplus::helpers::Properties p;
    p.setProperty(LOG4CPLUS_TEXT("ident"),
            LOG4CPLUS_STRING_TO_TSTRING(name));
    p.setProperty(LOG4CPLUS_TEXT("facility"),LOG4CPLUS_TEXT("local0"));

    log4cplus::SharedAppenderPtr appender( new log4cplus::SysLogAppender(p));
    appender->setLayout(
            std::auto_ptr<log4cplus::Layout>(new log4cplus::PatternLayout(
                    LOG4CPLUS_TEXT("%-5p %c <%x>: %m"))));

    log4cplus::Logger root = log4cplus::Logger::getRoot();
    root.addAppender(appender);
    root.setLogLevel(log4cplus::INFO_LOG_LEVEL);
}

/////////////////////////////////////////////////////////////////////////////
void Main::configureLogging(const PdServ::Config& config)
{
    if (!config) {
        syslogLogging();
        return;
    }

    typedef std::basic_istringstream<log4cplus::tchar> tistringstream;
    tistringstream is(LOG4CPLUS_STRING_TO_TSTRING(config.toString()));
    log4cplus::PropertyConfigurator(is).configure();
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

    const char *env = ::getenv("PDSERV_CONFIG");

    // Load custom configuration file
    if (!configFile.empty()) {
        const char *err = config.load(configFile.c_str());
        if (err)
            std::cout << "Load config: " << err << std::endl;
        else
            log_debug("Loaded specified configuration file %s",
                    configFile.c_str());
    }
    else if (env and ::strlen(env)) {
        // Try to load environment configuration file
        const char *err = config.load(env);

        if (err)
            std::cout << "Error loading config file " << env
                << ": " << err << std::endl;
        else
            log_debug("Loaded ENV config %s", env);
    }
    else {
        // Try to load default configuration file
        const char *f = QUOTE(SYSCONFDIR) "/pdserv.conf";
        const char *err = config.load(f);
        if (err) {
            if (::access(f, R_OK))
                log_debug("Could not access configuration file %s: %s",
                        f, ::strerror(errno));
            else
                std::cout << "Load config error: " << err << std::endl;
        }
        else
            log_debug("Loaded default configuration file %s", f);
    }

    if (!config)
        log_debug("No configuration loaded");

    // Only child runs after this point
    pid = getpid();

    // Go to root
#ifndef PDS_DEBUG
    ::chdir("/");
    ::umask(0777);
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
    while (fd_max--) {
#ifdef PDS_DEBUG
        if (fd_max == 2)
            break;
#endif
        ::close(fd_max);
    }

#ifndef PDS_DEBUG
    // Reopen STDIN, STDOUT and STDERR
    if (STDIN_FILENO == ::open("/dev/null", O_RDWR)) {
        dup2(STDIN_FILENO, STDOUT_FILENO);
        dup2(STDIN_FILENO, STDERR_FILENO);

        fcntl( STDIN_FILENO, F_SETFL, O_RDONLY);
        fcntl(STDOUT_FILENO, F_SETFL, O_WRONLY);
        fcntl(STDERR_FILENO, F_SETFL, O_WRONLY);
    }
#endif

    // Initialize non-real time tasks
    for (TaskList::iterator it = task.begin(); it != task.end(); ++it)
        static_cast<Task*>(*it)->nrt_init();

#ifdef PDS_DEBUG
    consoleLogging();
#endif

    // Do the logging configuration. Note that logging may open file handles
    configureLogging(config["logging"]);

    LOG4CPLUS_INFO(log4cplus::Logger::getRoot(),
            LOG4CPLUS_TEXT("Started application ")
            << LOG4CPLUS_STRING_TO_TSTRING(name)
            << LOG4CPLUS_TEXT(", Version ")
            << LOG4CPLUS_STRING_TO_TSTRING(version));

    PdServ::Main::startServers(config);

    // Hang here forever
    while (true) {
        pause();
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
        s->time = *t;
        s->rv = p->trigger(reinterpret_cast<struct pdtask *>(task),
                reinterpret_cast<const struct pdvariable *>(v),
                p->addr + s->offset,
                p->valueBuf + s->offset, s->count, p->priv_data);
        s->count = 0;
    }
}
