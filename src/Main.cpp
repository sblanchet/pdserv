/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "config.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "Main.h"
#include "Task.h"
#include "Signal.h"
#include "Parameter.h"
#include "Session.h"
#include "etlproto/Server.h"
#include "msrproto/Server.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace HRTLab;

/////////////////////////////////////////////////////////////////////////////
Main::Main(int argc, const char *argv[],
        const char *name, const char *version,
        double baserate,
        unsigned int nst, const unsigned int decimation[],
        int (*gettime)(struct timespec*)):
    name(name), version(version), baserate(baserate), nst(nst),
    decimation(nst > 1 ? new unsigned int [nst] : 0),
    gettime(gettime ? gettime : localtime), mutex(1), sdoMutex(1)
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
    delete parameterAddr;
    delete msrproto;

    for (SignalList::const_iterator it = signals.begin();
            it != signals.end(); it++)
        delete *it;

    for (ParameterList::const_iterator it = parameters.begin();
            it != parameters.end(); it++)
        delete *it;

    munmap(shmem, shmem_len);

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
unsigned int Main::writeParameter(const Parameter * const *p, size_t nelem,
        const char *data, int *errorCode)
{
    ost::SemaphoreLock lock(sdoMutex);

    size_t len = 0;
    for (size_t i = 0; i < nelem; i++)
        len += p[i]->memSize;

    for (size_t i = 0; i < nelem; i++)
        sdo->data[i].parameter = p[i];
    std::copy(data, data+len, sdoData);
    sdo->count = nelem;
    sdo->type = SDOStruct::WriteParameter;
    sdo->reqId++;

    do {
        ost::Thread::sleep(100);
    } while (sdo->reqId != sdo->replyId);

    for (size_t i = 0; i < nelem; i++) {

        if (errorCode)
            errorCode[i] = sdo->data[i].errorCode;

        if (!sdo->data[i].errorCode) {
            msrproto->parameterChanged(p, nelem);
            //etlproto->parameterChanged(p, nelem);
        }
    }

    return sdo->errorCount;
}

/////////////////////////////////////////////////////////////////////////////
void Main::processSdo(unsigned int tid, const struct timespec *time)
{
    // PollSignal
    // Receive:
    //          [0]: PollSignal
    //          [1]: The number of signals being polled (n)
    //          [2 .. 2+n]: Index numbers of the signals
    //
    // The reply is in the poll area

    if (sdo->reqId == sdo->replyId)
        return;

    sdo->time = *time;
    sdo->errorCount = 0;

    bool finished = true;
    char *data = sdoData;
    struct timespec t;
    switch (sdo->type) {
        case SDOStruct::PollSignal:
            for (unsigned i = 0; i < sdo->count; i++) {
                const Signal *s = sdo->data[i].signal;
                if (s->tid == tid)
                    std::copy((const char *)s->addr,
                            (const char *)s->addr + s->memSize,
                            data);
                else
                    finished = false;
                data += s->memSize;
            }
            break;

        case SDOStruct::WriteParameter:
            gettime(&t);
            for (unsigned idx = 0; idx != sdo->count; idx++) {
                const Parameter *p = sdo->data[idx].parameter;

                int errorCode = p->hrtSetValue(tid, data);
                data += p->memSize;
                sdo->data[idx].errorCode = errorCode;
                if (errorCode) {
                    sdo->errorCount++;
                }
                else {
                    mtime[p->index] = t;
                    std::copy((const char *)p->addr,
                            (const char *)p->addr + p->memSize,
                            parameterAddr[p->index]);
                }
            }
            break;
    }

    if (finished)
        sdo->replyId = sdo->reqId;
}

/////////////////////////////////////////////////////////////////////////////
void Main::update(int st, const struct timespec *time)
{
    processSdo(st, time);
    task[st]->txPdo(time);
}

/////////////////////////////////////////////////////////////////////////////
unsigned int Main::poll(const Signal * const *s, size_t nelem, char *buf)
{
    ost::SemaphoreLock lock(sdoMutex);

    for (unsigned i = 0; i < nelem; i++)
        sdo->data[i].signal = s[i];
    sdo->count = nelem;
    sdo->type = SDOStruct::PollSignal;
    sdo->reqId++;

    do {
        ost::Thread::sleep(100);
    } while (sdo->reqId != sdo->replyId);

    char *p = sdoData;
    while (nelem--) {
        size_t len = (*s)->memSize;

        std::copy(p, p + len, buf);
        p += len;
        buf += len;
        s++;
    }

    return sdo->errorCount;
}

/////////////////////////////////////////////////////////////////////////////
void Main::closeSession(const Session *s)
{
    ost::SemaphoreLock lock(mutex);
    for (unsigned int i = 0; i < nst; i++)
        task[i]->endSession(s);
}

/////////////////////////////////////////////////////////////////////////////
void Main::newSession(const Session* s)
{
    ost::SemaphoreLock lock(mutex);
    for (unsigned int i = 0; i < nst; i++)
        task[i]->newSession(s);
}

/////////////////////////////////////////////////////////////////////////////
const struct timespec *Main::getMTime(const Parameter *p) const
{
    return mtime + p->index;
}

/////////////////////////////////////////////////////////////////////////////
void Main::rxPdo(Session *session)
{
    for (unsigned int i = 0; i < nst; i++)
        task[i]->rxPdo(session);
}

/////////////////////////////////////////////////////////////////////////////
void Main::unsubscribe(const Session *session,
        const Signal * const *signal, size_t nelem)
{
    ost::SemaphoreLock lock(mutex);
//    cout << "Main::unsubscribe " << signal << endl;
    if (signal)
        for (const Signal * const *s = signal; s != signal + nelem; s++)
            task[(*s)->tid]->unsubscribe(session, *s);
    else
        for (unsigned int i = 0; i < nst; i++)
            task[i]->unsubscribe(session);
}

/////////////////////////////////////////////////////////////////////////////
void Main::subscribe(const Session *session,
        const Signal * const *signal, size_t nelem)
{
    ost::SemaphoreLock lock(mutex);
    for (const Signal * const *s = signal; s != signal + nelem; s++)
        task[(*s)->tid]->subscribe(session, *s);
}

/////////////////////////////////////////////////////////////////////////////
void Main::getSessionStatistics(std::list<Session::Statistics>& stats) const
{
    msrproto->getSessionStatistics(stats);
    //etlproto->getSessionStatistics(stats);
}

/////////////////////////////////////////////////////////////////////////////
template<class T>
T* ptr_align(void *p)
{
    const size_t mask = sizeof(T*) - 1;
    return reinterpret_cast<T*>(((unsigned)p + mask) & ~mask);
}

/////////////////////////////////////////////////////////////////////////////
int Main::start()
{
    for (unsigned int i = 0; i < nst; i++)
        task[i]->setup();

    std::list<Parameter*> p[9];
    size_t parameterSize = 0;
    for (ParameterList::const_iterator it = parameters.begin();
            it != parameters.end(); it++) {
        p[(*it)->width].push_back(*it);
        parameterSize += (*it)->memSize;
    }

    size_t signalSize = 0;
    for (SignalList::const_iterator it = signals.begin();
            it != signals.end(); it++) {
        signalSize += (*it)->memSize;
    }

    shmem_len =
        sizeof(*mtime) * parameters.size()
        + parameterSize + 8
        + sizeof(*sdo)
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

    mtime         = ptr_align<struct timespec>(shmem);
    parameterData = ptr_align<char>(mtime + parameters.size());
    sdo           = ptr_align<SDOStruct>(parameterData + parameterSize);
    sdoData       = ptr_align<char>(
            sdo->data + std::max(signals.size(), parameters.size()));
//    cout
//        << " shmem=" << shmem
//        << " mtime=" << mtime
//        << " parameterData=" << (void*)parameterData
//        << " sdo=" << sdo
//        << " sdoData=" << (void*)sdoData
//        << endl;

    parameterAddr = new char*[parameters.size()];

    const size_t idx[] = {8,4,2,1};
    char *buf = parameterData;
    for (size_t i = 0; i < 4; i++) {
        for (std::list<Parameter*>::const_iterator it = p[idx[i]].begin();
                it != p[idx[i]].end(); it++) {
            parameterAddr[(*it)->index] = buf;
            std::copy((const char *)(*it)->addr,
                    (const char *)(*it)->addr + (*it)->memSize, buf);
            buf += (*it)->memSize;
        }
    }

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
Signal *Main::newSignal(
        const char *path,
        enum si_datatype_t datatype,
        const void *addr,
        unsigned int tid,
        unsigned int decimation,
        unsigned int ndims,
        const size_t dim[]
        )
{
    if (tid >= nst)
        return 0;

    if (variableMap.find(path) != variableMap.end())
        return 0;

    Signal *s = new Signal(tid, decimation, path,
            datatype, addr, ndims, dim);

    task[tid]->addSignal(s);
    signals.push_back(s);
    variableMap[path] = s;

    return s;
}

/////////////////////////////////////////////////////////////////////////////
Parameter *Main::newParameter(
        const char *path,
        enum si_datatype_t datatype,
        void *addr,
        unsigned int mode,
        unsigned int ndims,
        const size_t dim[]
        )
{
    if (variableMap.find(path) != variableMap.end())
        return 0;

    Parameter *p = new Parameter(
            parameters.size(), path, mode, datatype, addr, ndims, dim);

    parameters.push_back(p);
    variableMap[path] = p;

    return p;
}
