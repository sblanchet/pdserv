#include "Main.h"
#include "Signal.h"
#include "Parameter.h"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/mman.h>

using namespace HRTLab;

/////////////////////////////////////////////////////////////////////////////
Main::Main(int argc, const char *argv[], unsigned int nst): nst(nst)
{
}

/////////////////////////////////////////////////////////////////////////////
Main::~Main()
{
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

    while (1) {
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

    signals.push_back(
            new Signal(tid, decimation, path, alias, datatype, ndims, dim, addr)
            );

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
    parameters.push_back(
            new Parameter(paramupdate, priv_data, path, alias, datatype, ndims, dim, addr)
            );

    return 0;
}
