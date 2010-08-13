#include "Main.h"
#include "Signal.h"
#include "Parameter.h"
#include "TCPServer.h"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/mman.h>

#include <cc++/socketport.h> 

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

using namespace HRTLab;

/////////////////////////////////////////////////////////////////////////////
Main::Main(int argc, const char *argv[],
        const char *name, const char *version,
        double baserate,
        unsigned int nst, const unsigned int decimation[]):
    name(name), version(version), baserate(baserate), nst(nst),
    decimation(nst > 1 ? new unsigned int [nst] : 0)
{
    if (nst > 1)
        memcpy((void*)this->decimation, decimation, sizeof(unsigned int [nst]));
    subscribers.resize(nst);
    blockLength.resize(nst);
}

/////////////////////////////////////////////////////////////////////////////
Main::~Main()
{
    delete decimation;
}

/////////////////////////////////////////////////////////////////////////////
unsigned int *Main::getSignalPtrStart() const
{
    return signal_ptr_start;
}

/////////////////////////////////////////////////////////////////////////////
const std::vector<Signal*>& Main::getSignals() const
{
    return signals;
}

/////////////////////////////////////////////////////////////////////////////
const std::vector<Parameter*>& Main::getParameters() const
{
    return parameters;
}

/////////////////////////////////////////////////////////////////////////////
const std::map<std::string,Variable*>& Main::getVariableMap() const
{
    return variableMap;
}

/////////////////////////////////////////////////////////////////////////////
void Main::update(int st)
{
    bool dirty[nst];
    memset(dirty, 0, sizeof(dirty));

    // Check for instructions in the inbox
    while (*instruction_ptr) {
        switch (*instruction_ptr++) {
            case Restart:
                instruction_ptr = instruction_block_begin;
                break;

            case Subscribe:
                {
                    unsigned int index = *instruction_ptr++;
                    Signal *s = signals[index];

                    s->subscriptionIndex = subscribers[s->tid].size();
                    subscribers[s->tid].push_back(index);
                    dirty[s->tid] = true;
                    blockLength[s->tid] += s->memSize;
                }
                break;

            case Unsubscribe:
                {
                    unsigned int index = *instruction_ptr++;
                    Signal *s = signals[index];

                    subscribers[s->tid].erase(
                            subscribers[s->tid].begin() + s->subscriptionIndex);
                    dirty[s->tid] = true;
                    blockLength[s->tid] -= s->memSize;
                }
                break;

            case SetValue:
                break;
        }
    }

    // Copy new signal lists to outbox
    for (unsigned int tid = 0; tid < nst; tid++) {
        if (!dirty[tid])
            continue;

        unsigned int *block_start = signal_ptr++;
        unsigned int n = subscribers[tid].size();

        if (signal_ptr + n + 3 >= signal_ptr_end) {
            *signal_ptr_start = 0;
            *block_start = Restart;
            signal_ptr = signal_ptr_start;
            block_start = signal_ptr++;
        }

        *signal_ptr++ = tid;
        *signal_ptr++ = n;

        std::copy(signal_ptr, signal_ptr+n, subscribers[tid].begin());
        signal_ptr += n;

        *signal_ptr = 0;
        *block_start = NewSubscriberList;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Main::subscribe(const std::string &path)
{
    VariableMap::iterator it = variableMap.find(path);
    if (it == variableMap.end() or !it->second->decimation)
        return;

    if (instruction_ptr + 2 >= instruction_ptr_end) {
        *instruction_ptr = Restart;
        instruction_ptr = instruction_block_begin;
    }

    instruction_ptr[1] = it->second->index;
    instruction_ptr[2] = 0;
    instruction_ptr[0] = Subscribe;
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

    ost::SocketService svc;
    ost::TCPSocket socket(ost::IPV4Address("0.0.0.0"), 4000);
    while (true) {
        new TCPServer(&svc, socket, this);
    }
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
        const void *addr
        )
{
    if (tid >= nst)
        return -EINVAL;

    if (variableMap.find(path) != variableMap.end())
        return -EEXIST;

    signals.push_back(
            new Signal(signals.size(), tid, decimation, path, alias,
                datatype, ndims, dim, addr)
            );
    variableMap[path] = signals.back();
    subscribers[tid].resize(subscribers[tid].size() + 1);

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
int Main::newParameter(
        paramupdate_t paramupdate,
        void *priv_data,
        const char *path,
        const char *alias,
        enum si_datatype_t datatype,
        unsigned int ndims,
        const size_t dim[],
        const void *addr
        )
{
    if (variableMap.find(path) != variableMap.end())
        return -EEXIST;

    parameters.push_back(
            new Parameter(parameters.size(), paramupdate, priv_data, path,
                alias, datatype, ndims, dim, addr)
            );
    variableMap[path] = parameters.back();

    return 0;
}
