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

void hrtlab_update(struct hrtlab* hrtlab, int st, const struct timespec *t)
{
    reinterpret_cast<HRTLab::Main*>(hrtlab)->update(st, t);
}

int hrtlab_signal(
        struct hrtlab* hrtlab,
        unsigned int tid,
        unsigned int decimation,
        const char *path,
        const char *alias,
        enum si_datatype_t datatype,
        size_t n,
        const size_t dim[],
        const void *addr
        )
{
    return reinterpret_cast<HRTLab::Main*>(hrtlab)->newSignal(
            tid, decimation, path, alias, datatype,
            dim ? n : 1, dim ? dim : &n,
            reinterpret_cast<const char*>(addr)
            );
}

int hrtlab_parameter(
        struct hrtlab* hrtlab,
        const char *path,
        const char *alias,
        enum si_datatype_t datatype,
        size_t n,
        const size_t dim[],
        void *addr,

        paramupdate_t paramcheck,
        paramupdate_t paramupdate,
        void *priv_data
        )
{
    return reinterpret_cast<HRTLab::Main*>(hrtlab)->newParameter(
            path, alias, datatype, dim ? n : 1, dim ? dim : &n,
            reinterpret_cast<char*>(addr),
            paramcheck, paramupdate, priv_data);
}

int hrtlab_start(struct hrtlab* hrtlab)
{
    return reinterpret_cast<HRTLab::Main*>(hrtlab)->start();
}

