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
int Main::setParameters(const HRTLab::Parameter * const *p,
        size_t count, const char *data) const
{
    ost::SemaphoreLock lock(sdoMutex);

    size_t delay = ~0U;

    for (size_t i = 0; i < nst; i++)
        delay = std::min(delay, (size_t)(task[i]->sampleTime * 1000 / 2));

    // Write the parameters into the shared memory sorted from widest
    // to narrowest data type width. This ensures that the data is
    // allways aligned correctly.
    size_t idx = 0;
    char *dst = sdoData;
    for (size_t width = HRTLab::Variable::maxWidth; width; width >>= 1) {
        const char *src = data;
        for (const HRTLab::Parameter * const* param = p;
                param != p + count; param++) {
            if ((*param)->width == width) {
                sdo->parameter[idx++] =
                    dynamic_cast<const Parameter*>((*param));
                std::copy(src, src + (*param)->memSize, dst);
                dst += (*param)->memSize;
            }
            src += (*param)->memSize;
        }
    }

    sdo->count = count;
    sdo->type = SDOStruct::WriteParameter;
    sdo->reqId++;

    do {
        ost::Thread::sleep(delay);
    } while (sdo->reqId != sdo->replyId);

    if (!sdo->errorCode) {
        const char *src = sdoData;
        for ( const Parameter **param = sdo->parameter;
                param != sdo->parameter + count; param++) {
            (*param)->copyValue(src, sdo->time);
            src += (*param)->memSize;
        }
        parametersChanged(p, count);
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

                data += p->memSize;

                if (sdo->errorCode) {
                    sdo->count = idx;
                    break;
                }
            }

            if (sdo->errorCode)
                break;

            data = sdoData;
            for (unsigned idx = 0; idx != sdo->count; idx++) {
                const Parameter *p = sdo->parameter[idx];

                (*p->trigger)(tid, 0, p->addr, data, p->memSize, p->priv_data);
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
        (*p)->getValue(buf, time ? time++ : 0);
        buf += (*p)->memSize;
        p++;
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
    struct timespec t = {0,0};

    size_t parameterSize = 0;
    for (HRTLab::Main::Parameters::iterator it = parameters.begin();
            it != parameters.end(); it++) {
        const Parameter *p = dynamic_cast<const Parameter*>(*it);
        p->copyValue(p->addr, t);
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

    cout
        << " shmem=" << shmem
        << " sdo=" << sdo
        << " sdoTaskTime=" << sdoTaskTime
        << " sdoData=" << (void*)sdoData
        << endl;

    char* buf = ptr_align<char>(sdoData + std::max(parameterSize, signalSize));
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

