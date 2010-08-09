#include "rtlab/rtlab.h"
#include "Main.h"

struct hrtlab* hrtlab_init(int argc, const char *argv[], 
        const char *name, const char *version, 
        double baserate, unsigned int nst, const unsigned int decimation[])
{
    return reinterpret_cast<struct hrtlab*>(
            new HRTLab::Main(argc, argv, name, version, baserate, nst,
                decimation));
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

int hrtlab_parameter(
        struct hrtlab* hrtlab,
        paramupdate_t paramupdate,
        void *priv_data,
        const char *path,
        const char *alias,
        enum si_datatype_t datatype,
        unsigned int ndims,
        const unsigned int dim[],
        const void *addr
        )
{
    return reinterpret_cast<HRTLab::Main*>(hrtlab)->newParameter(
            paramupdate, priv_data, path, alias, datatype, ndims, dim, addr
            );
}

int hrtlab_start(struct hrtlab* hrtlab)
{
    return reinterpret_cast<HRTLab::Main*>(hrtlab)->start();
}

