/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "config.h"

#include <cerrno>
#include <sys/mman.h>

#include <sys/types.h>
#include <unistd.h>             // fork()

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

/////////////////////////////////////////////////////////////////////////////
Main::Main(int argc, const char *argv[],
        const char *name, const char *version,
        double baserate,
        unsigned int nst, const unsigned int decimation[],
        int (*gettime)(struct timespec*)):
    HRTLab::Main(name, version, baserate, nst),
    task(new Task*[nst]), mutex(1), sdoMutex(1), rttime(gettime)
{
    for (size_t i = 0; i < nst; i++)
        task[i] =
            new Task(this, i, baserate * (decimation ? decimation[i] : 1));

    shmem_len = 0;
    shmem = 0;
}

/////////////////////////////////////////////////////////////////////////////
Main::~Main()
{
    for (ParameterList::const_iterator it = parameters.begin();
            it != parameters.end(); it++)
        delete *it;

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
int Main::setParameters(const HRTLab::Parameter * const *p, size_t nelem,
        const char *data) const
{
    ost::SemaphoreLock lock(sdoMutex);
    size_t delay = ~0U;

    for (size_t i = 0; i < nst; i++)
        delay = std::min(delay, (size_t)(task[i]->sampleTime * 1000 / 2));

    size_t len = 0;
    for (size_t i = 0; i < nelem; i++) {
        sdo->parameter[i] = dynamic_cast<const Parameter*>(p[i]);
        len += p[i]->memSize;
    }

    std::copy(data, data+len, sdoData);
    sdo->count = nelem;
    sdo->type = SDOStruct::WriteParameter;
    sdo->reqId++;

    do {
        ost::Thread::sleep(delay);
    } while (sdo->reqId != sdo->replyId);

    if (!sdo->errorCode) {
        for (size_t i = 0; i < nelem; i++)
            mtime[dynamic_cast<const Parameter*>(p[i])->index] = sdo->time;
        parametersChanged(p, nelem);
    }
    
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
            gettime(&sdo->time);
            for (unsigned idx = 0; idx != sdo->count; idx++) {
                const Parameter *p = sdo->parameter[idx];
                sdo->errorCode =
                    (*p->trigger)(tid, 1, 0, data, p->memSize, p->priv_data);
                cout << "SDOStruct::WriteParameter:check " << p->path
                    << " errorcode " << sdo->errorCode << endl;

                data += p->memSize;

                if (sdo->errorCode) {
                    sdo->count = idx;
                    break;
                }
            }

            cout << "SDOStruct::WriteParameter: errorcode " << sdo->errorCode << endl;
            if (sdo->errorCode)
                break;

            data = sdoData;
            for (unsigned idx = 0; idx != sdo->count; idx++) {
                const Parameter *p = sdo->parameter[idx];
                cout << "SDOStruct::WriteParameter:copy " << p->path << endl;

                (*p->trigger)(tid, 0, p->addr, data, p->memSize, p->priv_data);
                std::copy(data, data + p->memSize, p->shmemAddr);
                data += p->memSize;
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
void Main::getValues(const HRTLab::Parameter * const *p, size_t nelem,
                char *buf, struct timespec *time) const
{
    ost::SemaphoreLock lock(sdoMutex);
    while (nelem--) {
        const Parameter *param = dynamic_cast<const Parameter *>(*p++);
        std::copy(param->shmemAddr, param->shmemAddr + param->memSize, buf);
        buf += param->memSize;
        if (time)
            *time++ = mtime[param->index];
    }
}

/////////////////////////////////////////////////////////////////////////////
void Main::getValues( const HRTLab::Signal * const *s, size_t nelem,
        char *buf, struct timespec *time) const
{
    ost::SemaphoreLock lock(sdoMutex);
    size_t delay = 10;
    size_t dataSize = 0;

    for (unsigned i = 0; i < nelem; i++) {
        sdo->signal[i] = dynamic_cast<const Signal*>(s[i]);
        dataSize += s[i]->memSize;
        delay = std::max(delay, (size_t)(s[i]->task->sampleTime * 1000 / 2));
    }
    sdo->count = nelem;
    sdo->type = SDOStruct::PollSignal;
    sdo->reqId++;

    do {
        ost::Thread::sleep(delay);
    } while (sdo->reqId != sdo->replyId);

    std::copy(sdoData, sdoData + dataSize, buf);
    while (time and nelem--)
        *time++ = sdoTaskTime[(*s++)->task->tid];
}

/////////////////////////////////////////////////////////////////////////////
int Main::init()
{
    size_t taskMemSize[nst];

    for (unsigned int i = 0; i < nst; i++) {
        taskMemSize[i] = ptr_align(task[i]->getShmemSpace(bufferTime));
        shmem_len += taskMemSize[i];
    }

    std::list<Parameter*> p[HRTLab::Variable::maxWidth+1];
    size_t parameterSize = 0;
    for (ParameterList::const_iterator it = parameters.begin();
            it != parameters.end(); it++) {
        p[(*it)->width].push_back(*it);
        parameterSize += (*it)->memSize;
    }

    size_t signalSize = 0;
    const HRTLab::Main::Signals& signals = getSignals();
    for (HRTLab::Main::Signals::const_iterator it = signals.begin();
            it != signals.end(); it++) {
        signalSize += (*it)->memSize;
    }

    size_t sdoCount = std::max(signals.size(), parameters.size());

    shmem_len +=
        sizeof(*mtime) * parameters.size()
        + parameterSize + 8
        + sizeof(*sdo) * sdoCount
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

    mtime         = ptr_align<struct timespec>(shmem);
    parameterData = ptr_align<char>(mtime + parameters.size());
    sdo           = ptr_align<SDOStruct>(parameterData + parameterSize);
    sdoTaskTime   = ptr_align<struct timespec>(sdo->signal + sdoCount);
    sdoData       = ptr_align<char>(sdoTaskTime + nst);

    cout
        << " shmem=" << shmem
        << " mtime=" << mtime
        << " parameterData=" << (void*)parameterData
        << " sdo=" << sdo
        << " sdoTaskTime=" << sdoTaskTime
        << " sdoData=" << (void*)sdoData
        << endl;

    char *buf = parameterData;
    for (size_t w = 8; w; w >>= 1) {
        std::list<Parameter*>::iterator it;
        for (it = p[w].begin(); it != p[w].end(); it++) {
            (*it)->shmemAddr = buf;
            std::copy((const char *)(*it)->addr,
                    (const char *)(*it)->addr + (*it)->memSize, buf);
            buf += (*it)->memSize;
        }
    }

    buf = ptr_align<char>(sdoData + std::max(parameterSize, signalSize));
    for (unsigned int i = 0; i < nst; i++) {
        task[i]->init(buf, buf + taskMemSize[i]);
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
        return 0;
    }

    pid = getpid();

    return startProtocols();
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

    return
        task[tid]->newSignal( path, datatype, addr, decimation, ndims, dim);
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
    Parameter *p = new Parameter(this, parameters.size(), path, mode, datatype, addr, ndims, dim);
    parameters.push_back(p);

    return p;
}

/////////////////////////////////////////////////////////////////////////////
HRTLab::Receiver *Main::newReceiver(unsigned int tid)
{
    return task[tid]->newReceiver();
}

/////////////////////////////////////////////////////////////////////////////
void Main::subscribe(const HRTLab::Session *session,
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
void Main::unsubscribe(const HRTLab::Session *session,
        const HRTLab::Signal * const *s, size_t n) const
{
    for (Task ** t = task; t != task + nst; t++)
        (*t)->unsubscribe(session, s, n);
}

