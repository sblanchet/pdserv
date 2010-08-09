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
}

/////////////////////////////////////////////////////////////////////////////
Main::~Main()
{
    delete decimation;
}

/////////////////////////////////////////////////////////////////////////////
void Main::update(int st)
{
}

/////////////////////////////////////////////////////////////////////////////
int Main::start()
{
    shmem_len = 1000000;

    shmem = ::mmap(0, shmem_len, PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_ANON, -1, 0);
    if (MAP_FAILED == shmem) {
        // log(LOGCRIT, "could not mmap
        // err << "mmap(): " << strerror(errno);
        ::perror("mmap()");
        return -errno;
    }
    ::memset(shmem, 0, shmem_len);

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
            new Signal(tid, decimation, path, alias, datatype, ndims, dim, addr)
            );
    variableMap[path] = parameters.back();

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
            new Parameter(paramupdate, priv_data, path, alias, datatype, ndims, dim, addr)
            );
    variableMap[path] = parameters.back();

    return 0;
}
