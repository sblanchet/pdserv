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
    gettime(gettime ? gettime : localtime), mutex(1), pollMutex(1),
    mtask(nst)
{
    if (nst > 1)
        std::copy(decimation, decimation + nst, this->decimation);

    mtask.resize(nst);

    task = new Task*[nst];
    for (size_t i = 0; i < nst; i++)
        task[i] = new Task(this, baserate * (decimation ? decimation[i] : 1));
}

/////////////////////////////////////////////////////////////////////////////
Main::~Main()
{
    delete decimation;
    for (size_t i = 0; i < nst; i++)
        delete mtask[i];
    delete[] subscriptionPtr;

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
const void * Main::getSignalPtrStart() const
{
    return signal_ptr_start;
}

/////////////////////////////////////////////////////////////////////////////
void Main::post(Instruction instr)
{
    ost::SemaphoreLock lock(mutex);

    if (instruction_ptr + 1 >= instruction_ptr_end) {
        *instruction_block_begin = 0;
        *instruction_ptr = Restart;
        instruction_ptr = instruction_block_begin;
    }

    instruction_ptr[1] = 0;
    instruction_ptr[0] = instr;
    instruction_ptr += 1;
}

/////////////////////////////////////////////////////////////////////////////
void Main::post(Instruction instr, unsigned int param,
        const char *buf, size_t len)
{
    ost::SemaphoreLock lock(mutex);

    size_t n = 2 + element_count_uint(len);

    if (instruction_ptr + n >= instruction_ptr_end) {
        *instruction_block_begin = 0;
        *instruction_ptr = Restart;
        instruction_ptr = instruction_block_begin;
    }

    instruction_ptr[1] = param;
    std::copy(buf, buf + len, reinterpret_cast<char*>(instruction_ptr+2));

    instruction_ptr[n] = 0;
    instruction_ptr[0] = instr;
    instruction_ptr += n;
}

/////////////////////////////////////////////////////////////////////////////
void Main::writeParameter(Parameter *p)
{
    post(SetValue, p->index, p->Variable::addr, p->memSize);

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
void Main::processPollSignal(const struct timespec *time)
{
    // PollSignal
    // Receive:
    //          [0]: PollSignal
    //          [1]: The number of signals being polled (n)
    //          [2 .. 2+n]: Index numbers of the signals
    //
    // The reply is in the poll area

    size_t signal_count = *instruction_ptr++;

    char *p = reinterpret_cast<char*>(pollId + 1);

    // struct timeval
    if (time)
        *reinterpret_cast<struct timespec*>(p) = *time;
    else
        std::fill_n(p, sizeof(struct timespec), 0);
    p += sizeof(*time);

    // Signal data
    while (signal_count--) {
        const Signal *s = signals[*instruction_ptr++];
        std::copy(s->addr, s->addr + s->memSize, p);
        p += s->memSize;
    }

    // Complete the message
    (*pollId)++;
}

/////////////////////////////////////////////////////////////////////////////
void Main::processSubscribe()
{
    unsigned int signal_count = *instruction_ptr++;

    for (unsigned int i = 0; i < signal_count; i++) {
        Signal *s = signals[*instruction_ptr++];
        mtask[s->tid]->subscribe(s->index);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Main::processUnsubscribe()
{
    unsigned int signal_count = *instruction_ptr++;

    for (unsigned int i = 0; i < signal_count; i++) {
        Signal *s = signals[*instruction_ptr++];
        mtask[s->tid]->unsubscribe(s->index);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Main::processSetValue()
{
    unsigned int index = *instruction_ptr++;
    Parameter *p = parameters[index];
    const char *data = 
        reinterpret_cast<const char*>(instruction_ptr);

    p->setValue(data);

    // Skip over the block with parameter data
    instruction_ptr += element_count_uint(p->memSize);
}

/////////////////////////////////////////////////////////////////////////////
void Main::update(int st, const struct timespec *time)
{
    // Check for instructions in the inbox
    while (*instruction_ptr) {
        switch (*instruction_ptr++) {
            case Restart:
                instruction_ptr = instruction_block_begin;
                break;

            case PollSignal:
                processPollSignal(time);
                break;

            case Subscribe:
                processSubscribe();
                break;

            case Unsubscribe:
                processUnsubscribe();
                break;

            case SetValue:
                processSetValue();
                break;
        }
    }

    mtask[st]->update(time);
    task[st]->txPdo(time);
}

/////////////////////////////////////////////////////////////////////////////
void Main::poll(Signal **s, size_t nelem, char *buf)
{
    ost::SemaphoreLock lock(pollMutex);
    unsigned int id = *pollId;
    {
        ost::SemaphoreLock globallock(mutex);
        const size_t blockLen = nelem + 2;

        if (instruction_ptr + blockLen >= instruction_ptr_end) {
            *instruction_block_begin = 0;
            *instruction_ptr = Restart;
            instruction_ptr = instruction_block_begin;
        }

        instruction_ptr[1] = nelem;
        for (size_t i = 0; i < nelem; i++)
            instruction_ptr[i+2] = s[i]->index;

        instruction_ptr[blockLen] = 0;
        instruction_ptr[0] = PollSignal;
        instruction_ptr += blockLen;
    }

    do {
        ost::Thread::sleep(100);
    } while (*pollId == id);

    const char *p = reinterpret_cast<const char*>(pollId + 1);

    p += sizeof(struct timeval);
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
    for (const Signal **s = signal; s != signal + nelem; s++)
        task[(*s)->tid]->unsubscribe(session, *s);
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

    pollId = reinterpret_cast<unsigned int*>(shmem);

    instruction_block_begin = pollId + 100;
    instruction_ptr     = instruction_block_begin;
    instruction_ptr_end = instruction_block_begin + 1024;
    *instruction_block_begin = 0;

    signal_ptr_start = instruction_ptr_end + 1;
    signal_ptr = signal_ptr_start;
    signal_ptr_end = reinterpret_cast<unsigned int*>(shmem+shmem_len);

    pid = ::fork();
    if (pid < 0) {
        // Some error occurred
        ::perror("fork()");
        return pid;
    }
    else if (pid) {
        // Parent here
        for (size_t i = 0; i < nst; i++)
            mtask[i] = new MTask(this, i);

        subscriptionPtr = new MTask::CopyList*[signals.size()];
        for (size_t i = 0; i < signals.size(); i++)
            subscriptionPtr[i] = 0;

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
