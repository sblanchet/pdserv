#include "rtlab/rtlab.h"
#include "Main.h"

struct hrtlab* hrtlab_init(int argc, const char *argv[], int nst)
{
    return reinterpret_cast<struct hrtlab*>(new HRTLab::Main(argc, argv, nst));
}

void hrtlab_exit(struct hrtlab* hrtlab)
{
    delete reinterpret_cast<HRTLab::Main*>(hrtlab);
}

void hrtlab_update(struct hrtlab* hrtlab, int st)
{
    reinterpret_cast<HRTLab::Main*>(hrtlab)->update(st);
}

int hrtlab_signal(
        struct hrtlab* hrtlab,
        unsigned int tid,
        unsigned int decimation,
        const char *path,
        const char *alias,
        enum si_datatype_t datatype,
        unsigned int ndims,
        const unsigned int dim[],
        const void *addr
        )
{
    return reinterpret_cast<HRTLab::Main*>(hrtlab)->newSignal(
            tid, decimation, path, alias, datatype, ndims, dim, addr
            );
}
