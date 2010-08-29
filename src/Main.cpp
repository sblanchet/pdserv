#include "Main.h"
#include "Signal.h"
#include "Parameter.h"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "etlproto/Server.h"
#include "MsrServer.h"

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
    gettime(gettime ? gettime : localtime)
{
    if (nst > 1)
        std::copy(decimation, decimation + nst, this->decimation);
    subscribers.resize(nst);
    blockLength.resize(nst);
    iterationNo.resize(nst);
    subscribed.resize(nst);
    subscriptionIndex.resize(nst);
    subscriptionMap.resize(nst);

    // the blockLength is at least the size of timespec
    std::fill_n(blockLength.begin(), nst, sizeof(struct timespec));

    std::vector<std::map<uint8_t, size_t> > signalCount;
    signalCount.resize(nst);
    for (std::vector<Signal*>::iterator it = signals.begin();
            it != signals.end(); it++) {
        signalCount[(*it)->tid][(*it)->width]++;
    }

    for (unsigned int i = 0; i < nst; i++) {
        for (std::map<uint8_t, size_t>::iterator it = signalCount[i].begin();
                it != signalCount[i].end(); it++)
            subscriptionMap[i][it->first].reserve(it->second);
    }

    subscribers.reserve(signals.size());
}

/////////////////////////////////////////////////////////////////////////////
Main::~Main()
{
    delete decimation;
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
unsigned int *Main::getSignalPtrStart() const
{
    return signal_ptr_start;
}

/////////////////////////////////////////////////////////////////////////////
void Main::post(Instruction instr)
{
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
    size_t n = (len + sizeof(*instruction_ptr) - 1)
        / sizeof(*instruction_ptr)
        + 2;

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
    // FIXME: servers->parameterChanged(p)
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
void Main::update(int st, const struct timespec *time)
{
    bool dirty[nst];
    std::fill_n(dirty, nst, false);

    // Check for instructions in the inbox
    while (*instruction_ptr) {
        switch (*instruction_ptr++) {
            case Restart:
                {
                    instruction_ptr = instruction_block_begin;
                }
                break;

            case SubscriptionList:
                {
                    std::fill_n(dirty, nst, true);
                }
                break;

            case Subscribe:
                {
                    unsigned int index = *instruction_ptr++;
                    unsigned int i = 0;
                    Signal *s = signals[index];

                    if (subscribed[index])
                        break;

                    subscribed[index] = true;
                    subscribers[s->tid].resize(subscribers[s->tid].size() + 1);

                    subscriptionIndex[index] =
                        subscriptionMap[s->tid][s->width].size();
                    subscriptionMap[s->tid][s->width].push_back(s);
                    for (SignalWidthMap::iterator it = subscriptionMap[s->tid].begin();
                            it != subscriptionMap[s->tid].end(); it++) {
                        for (std::vector<Signal*>::iterator it2 = it->second.begin();
                                it2 != it->second.end(); it2++) {
                            subscribers[s->tid][i] = *it2;
                            i++;
                        }
                    }
                    dirty[s->tid] = true;
                    blockLength[s->tid] += s->memSize;
                }
                break;

            case Unsubscribe:
                {
                    unsigned int index = *instruction_ptr++;
                    Signal *s = signals[index];

                    if (subscribed[index]) {
                        subscribers[s->tid].erase(
                                subscribers[s->tid].begin()
                                + subscriptionIndex[index]);

                        std::vector<Signal*>::iterator it = 
                            subscriptionMap[s->tid][s->width].begin()
                            + subscriptionIndex[index];
                        subscriptionMap[s->tid][s->width].erase(it);

                        dirty[s->tid] = true;
                        blockLength[s->tid] -= s->memSize;
                        subscribed[index] = false;
                    }
                }
                break;

            case SetValue:
                {
                    unsigned int index = *instruction_ptr++;
                    Parameter *p = parameters[index];

                    p->setValue(
                            reinterpret_cast<const char*>(instruction_ptr));

                }
                break;
        }
    }

    // Copy new signal lists to outbox
    for (unsigned int tid = 0; tid < nst; tid++) {
        if (!dirty[tid])
            continue;

        size_t n = subscribers[tid].size();
        size_t headerLen = 4;

        if (signal_ptr + n + headerLen >= signal_ptr_end) {
            *signal_ptr_start = 0;
            signal_ptr[0] = Restart;
            signal_ptr = signal_ptr_start;
        }

        signal_ptr[1] = n + headerLen;  // Block size in (unsigned int)
        signal_ptr[2] = tid;            // Task Id
        signal_ptr[3] = 1;              // Transmission decimation

        for (unsigned int i = 0; i < n; i++)
            signal_ptr[i + headerLen] = subscribers[tid][i]->index;

        signal_ptr[n + headerLen] = 0;
        signal_ptr[0] = NewSubscriberList;

        signal_ptr += n + headerLen;
    }

    // Copy signals over
    //if (!subscribers[st].empty())
    {
        size_t headerLen = 4;
        size_t n =
            (blockLength[st] + sizeof(unsigned int) - 1)
            / sizeof(unsigned int);

        if (signal_ptr + n + headerLen >= signal_ptr_end) {
            *signal_ptr_start = 0;
            signal_ptr[0] = Restart;
            signal_ptr = signal_ptr_start;
        }

        signal_ptr[1] = n + headerLen;      // Block size in (unsigned int)
        signal_ptr[2] = st;                 // Task Id
        signal_ptr[3] = iterationNo[st]++;  // Iteration number
        char *dataPtr = reinterpret_cast<char*>(&signal_ptr[headerLen]);

        if (time)
            *reinterpret_cast<struct timespec*>(dataPtr) = *time;
        else
            std::fill_n(dataPtr, sizeof(struct timespec), 0);
        dataPtr += sizeof(struct timespec);                 // TimeSpec

        for (std::vector<Signal*>::iterator it = subscribers[st].begin();
                it != subscribers[st].end(); it++) {
            std::copy((*it)->addr, (*it)->addr + (*it)->memSize, dataPtr);
            dataPtr += (*it)->memSize;
        }
        signal_ptr[n + headerLen] = 0;
        signal_ptr[0] = SubscriptionData;
        signal_ptr += n + headerLen;
    }
}

/////////////////////////////////////////////////////////////////////////////
unsigned int Main::subscribe(const std::string &path)
{
    VariableMap::iterator it = variableMap.find(path);
    if (!path.empty()
            and (it == variableMap.end() or !it->second->decimation))
        return ~0U;

    if (path.empty()) {
        post(SubscriptionList);
    }
    else {
        post(Subscribe, it->second->index);

        return it->second->index;
    }

    return ~0U;
}

/////////////////////////////////////////////////////////////////////////////
int Main::start()
{
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

    instruction_block_begin = reinterpret_cast<unsigned int*>(shmem);
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
        return 0;
    }

    MsrProto::Server msrproto(this);
//    EtlProto::Server etlproto(this);
//    etlproto.start();
    msrproto.start();

//    etlproto.join();
    msrproto.join();

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

    Signal *s = new Signal(signals.size(), tid, decimation, path, alias,
                datatype, ndims, dim, addr);
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
