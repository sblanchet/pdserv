#include "Main.h"
#include "Task.h"
#include "Signal.h"
#include "Parameter.h"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <list>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "etlproto/Server.h"
#include "msrproto/Server.h"
#include "Session.h"

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

using namespace HRTLab;

/////////////////////////////////////////////////////////////////////////////
Main::Main(int argc, const char *argv[],
        const char *name, const char *version,
        double baserate,
        unsigned int nst, const unsigned int decimation[],
        int (*gettime)(struct timespec*)):
    name(name), version(version), baserate(baserate), nst(nst),
    decimation(nst > 1 ? new unsigned int [nst] : 0),
    gettime(gettime ? gettime : localtime), mutex(1), pollMutex(1)
{
    if (nst > 1)
        std::copy(decimation, decimation + nst, this->decimation);

    task = new Task*[nst];
    for (size_t i = 0; i < nst; i++)
        task[i] =
            new Task(this, i, baserate * (decimation ? decimation[i] : 1));
}

/////////////////////////////////////////////////////////////////////////////
Main::~Main()
{
    delete decimation;

    for (size_t i = 0; i < nst; i++)
        delete task[i];
    delete[] task;
}

/////////////////////////////////////////////////////////////////////////////
int Main::localtime(struct timespec* t)
{
    struct timeval tv;

    if (gettimeofday(&tv, 0))
        return errno;
    t->tv_sec = tv.tv_sec;
    t->tv_nsec = tv.tv_usec * 1000;

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
void Main::writeParameter(Parameter *p)
{
//    post(SetValue, p->index, p->Variable::addr, p->memSize);

    msrproto->parameterChanged(p);
}

/////////////////////////////////////////////////////////////////////////////
const Main::SignalList& Main::getSignals() const
{
    return signals;
}

/////////////////////////////////////////////////////////////////////////////
const Main::ParameterList& Main::getParameters() const
{
    return parameters;
}

/////////////////////////////////////////////////////////////////////////////
const Main::VariableMap& Main::getVariableMap() const
{
    return variableMap;
}

/////////////////////////////////////////////////////////////////////////////
void Main::processPollSignal(unsigned int tid, const struct timespec *time)
{
    // PollSignal
    // Receive:
    //          [0]: PollSignal
    //          [1]: The number of signals being polled (n)
    //          [2 .. 2+n]: Index numbers of the signals
    //
    // The reply is in the poll area

    if (pollStruct->reqId == pollStruct->replyId)
        return;

    pollStruct->time = *time;

    bool finished = true;
    char *p = pollData;
    for (const Signal **s = pollStruct->signals; *s; s++) {
        if ((*s)->tid == tid)
            std::copy((*s)->addr, (*s)->addr + (*s)->memSize, p);
        else
            finished = false;
        p += (*s)->memSize;
    }

    if (finished)
        pollStruct->replyId = pollStruct->reqId;
}

/////////////////////////////////////////////////////////////////////////////
void Main::processSetValue()
{
//    unsigned int index = *instruction_ptr++;
//    Parameter *p = parameters[index];
//    const char *data = 
//        reinterpret_cast<const char*>(instruction_ptr);
//
//    p->setValue(data);
//
//    // Skip over the block with parameter data
//    instruction_ptr += element_count_uint(p->memSize);
}

/////////////////////////////////////////////////////////////////////////////
void Main::update(int st, const struct timespec *time)
{
    processPollSignal(st, time);
    task[st]->txPdo(time);
}

/////////////////////////////////////////////////////////////////////////////
void Main::poll(Signal **s, size_t nelem, char *buf)
{
    ost::SemaphoreLock lock(pollMutex);

    std::copy(s, s+nelem, pollStruct->signals);
    pollStruct->count = nelem;
    pollStruct->reqId++;

    do {
        ost::Thread::sleep(100);
    } while (pollStruct->reqId != pollStruct->replyId);

    char *p = pollData;
    while (nelem--) {
        size_t len = (*s)->memSize;

        std::copy(p, p + len, buf);
        p += len;
        buf += len;
        s++;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Main::closeSession(const Session *s)
{
    ost::SemaphoreLock lock(mutex);
    session.erase(s);
    for (unsigned int i = 0; i < nst; i++)
        task[i]->endSession(s);
}

/////////////////////////////////////////////////////////////////////////////
void Main::newSession(const Session* s)
{
    ost::SemaphoreLock lock(mutex);
    session.insert(s);

    for (unsigned int i = 0; i < nst; i++)
        task[i]->newSession(s);
}

/////////////////////////////////////////////////////////////////////////////
void Main::rxPdo(Session *session)
{
    for (unsigned int i = 0; i < nst; i++)
        task[i]->rxPdo(session);
}

/////////////////////////////////////////////////////////////////////////////
void Main::unsubscribe(const Session *session,
        const Signal **signal, size_t nelem)
{
    ost::SemaphoreLock lock(mutex);
    cout << "Main::unsubscribe " << signal << endl;
    if (signal)
        for (const Signal **s = signal; s != signal + nelem; s++)
            task[(*s)->tid]->unsubscribe(session, *s);
    else
        for (unsigned int i = 0; i < nst; i++)
            task[i]->unsubscribe(session);
}

/////////////////////////////////////////////////////////////////////////////
void Main::subscribe(const Session *session,
        const Signal **signal, size_t nelem)
{
    ost::SemaphoreLock lock(mutex);
    for (const Signal **s = signal; s != signal + nelem; s++)
        task[(*s)->tid]->subscribe(session, *s);
}

/////////////////////////////////////////////////////////////////////////////
void Main::getSessionStatistics(std::list<SessionStatistics>& stats) const
{
    ost::SemaphoreLock lock(mutex);
    for (SessionSet::const_iterator it = session.begin();
            it != session.end(); it++) {
        SessionStatistics ss;
        (*it)->getSessionStatistics(ss);
        stats.push_back(ss);
    }
}

/////////////////////////////////////////////////////////////////////////////
int Main::start()
{
    for (unsigned int i = 0; i < nst; i++)
        task[i]->setup();

    shmem_len = 1000000;

    shmem = (char*)::mmap(0, shmem_len, PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_ANON, -1, 0);
    if (MAP_FAILED == shmem) {
        // log(LOGCRIT, "could not mmap
        // err << "mmap(): " << strerror(errno);
        ::perror("mmap()");
        return -errno;
    }
    ::memset(shmem, 0, shmem_len);

    pollStruct = reinterpret_cast<PollStruct*>(shmem);
    pollData =
        reinterpret_cast<char*>(pollStruct->signals + variableMap.size());

    pid = ::fork();
    if (pid < 0) {
        // Some error occurred
        ::perror("fork()");
        return pid;
    }
    else if (pid) {
        // Parent here
        return 0;
    }

    msrproto = new MsrProto::Server(this);
//    EtlProto::Server etlproto(this);
//    etlproto.start();
    msrproto->start();

//    etlproto.join();
    msrproto->join();

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
int Main::newSignal(
        unsigned int tid,
        unsigned int decimation,
        const char *path,
        const char *alias,
        enum si_datatype_t datatype,
        unsigned int ndims,
        const size_t dim[],
        const char *addr
        )
{
    if (tid >= nst)
        return -EINVAL;

    if (variableMap.find(path) != variableMap.end())
        return -EEXIST;

    Signal *s = new Signal(tid, signals.size(), decimation, path, alias,
            datatype, ndims, dim, addr);

    task[tid]->addVariable(s);
    signals.push_back(s);
    variableMap[path] = s;

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
int Main::newParameter(
        const char *path,
        const char *alias,
        enum si_datatype_t datatype,
        unsigned int ndims,
        const size_t dim[],
        char *addr,
        paramupdate_t paramcheck,
        paramupdate_t paramupdate,
        void *priv_data
        )
{
    if (variableMap.find(path) != variableMap.end())
        return -EEXIST;

    parameters.push_back(
            new Parameter( this, parameters.size(),
                path, alias, datatype, ndims, dim, addr,
                paramcheck, paramupdate, priv_data));
    variableMap[path] = parameters.back();

    return 0;
}
