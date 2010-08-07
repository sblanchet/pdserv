#include "Main.h"
#include "Signal.h"
#include <cerrno>

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
