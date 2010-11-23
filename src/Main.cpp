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
int Main::writeParameter(const Parameter * const *p, size_t nelem,
        const char *data)
{
    ost::SemaphoreLock lock(sdoMutex);

    size_t len = 0;
    for (size_t i = 0; i < nelem; i++)
        len += p[i]->memSize;

    std::copy(p, p+nelem, sdo->parameters);
    std::copy(data, data+len, sdoData);
    sdo->count = nelem;
    sdo->type = SDOStruct::WriteParameter;
    sdo->reqId++;

    do {
        ost::Thread::sleep(100);
    } while (sdo->reqId != sdo->replyId);

    if (!sdo->errorCode) {
        msrproto->parameterChanged(p, nelem);
        //etlproto->parameterChanged(p, nelem);
    }

    return sdo->errorCode;
}

/////////////////////////////////////////////////////////////////////////////
const char * Main::getParameterAddr(const Parameter *p) const
{
    return parameterAddr[p->index];
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
    sdo->errorCode = 0;

    bool finished = true;
    char *data = sdoData;
    struct timespec t;
    switch (sdo->type) {
        case SDOStruct::PollSignal:
            for (const Signal **s = sdo->signals;
                    s != sdo->signals + sdo->count; s++) {
                if ((*s)->tid == tid)
                    std::copy((*s)->addr, (*s)->addr + (*s)->memSize, data);
                else
                    finished = false;
                data += (*s)->memSize;
            }
            break;

        case SDOStruct::WriteParameter:
            gettime(&t);
            for (const Parameter **p = sdo->parameters;
                    p != sdo->parameters + sdo->count; p++) {

                int errorCode = (*p)->setValue(tid, data);
                if (errorCode) {
                    *p = 0;
                    sdo->errorCode = errorCode;
                }
                else {
                    mtime[(*p)->index] = t;
                    std::copy((*p)->Variable::addr,
                            (*p)->Variable::addr + (*p)->memSize,
                            parameterAddr[(*p)->index]);
                }

                data += (*p)->memSize;
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
int Main::poll(const Signal * const *s, size_t nelem, char *buf)
{
    ost::SemaphoreLock lock(sdoMutex);

    std::copy(s, s+nelem, sdo->signals);
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

    return sdo->errorCode;
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
            sdo->signals + std::max(signals.size(), parameters.size()));
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
            std::copy((*it)->Variable::addr,
                    (*it)->Variable::addr + (*it)->memSize, buf);
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
int Main::newSignal(
        unsigned int tid,
        unsigned int decimation,
        const char *path,
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

    Signal *s = new Signal(tid, signals.size(), decimation, path,
            datatype, ndims, dim, addr);

    task[tid]->addVariable(s);
    signals.push_back(s);
    variableMap[path] = s;

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
int Main::newParameter(
        const char *path,
        unsigned int mode,
        enum si_datatype_t datatype,
        unsigned int ndims,
        const size_t dim[],
        char *addr,
        paramupdate_t paramcopy,
        void *priv_data
        )
{
    if (variableMap.find(path) != variableMap.end())
        return -EEXIST;

    Parameter *p = new Parameter(parameters.size(), path, mode, datatype,
            ndims, dim, addr, paramcopy, priv_data);

    parameters.push_back(p);
    variableMap[path] = p;

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
int Main::setAlias( const char *path, const char *alias)
{
    VariableMap::iterator it = variableMap.find(path);

    if (it == variableMap.end())
        return -EOF;

    it->second->alias = alias;
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
int Main::setUnit( const char *path, const char *unit)
{
    VariableMap::iterator it = variableMap.find(path);

    if (it == variableMap.end())
        return -EOF;

    it->second->unit = unit;
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
int Main::setComment( const char *path, const char *comment)
{
    VariableMap::iterator it = variableMap.find(path);

    if (it == variableMap.end())
        return -EOF;

    it->second->comment = comment;
    return 0;
}
