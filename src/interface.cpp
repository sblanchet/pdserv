/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "rtlab/rtlab.h"
#include "Main.h"

/////////////////////////////////////////////////////////////////////////////
struct hrtlab* hrtlab_init(int argc, const char *argv[], 
        const char *name, const char *version, 
        double baserate, unsigned int nst, const unsigned int decimation[],
        int (*gettime)(struct timespec*))
{
    return reinterpret_cast<struct hrtlab*>(
            new HRTLab::Main(argc, argv, name, version, baserate, nst,
                decimation, gettime));
}

/////////////////////////////////////////////////////////////////////////////
void hrtlab_exit(struct hrtlab* hrtlab)
{
    delete reinterpret_cast<HRTLab::Main*>(hrtlab);
}

/////////////////////////////////////////////////////////////////////////////
void hrtlab_update(struct hrtlab* hrtlab, int st, const struct timespec *t)
{
    reinterpret_cast<HRTLab::Main*>(hrtlab)->update(st, t);
}

/////////////////////////////////////////////////////////////////////////////
int hrtlab_signal(
        struct hrtlab* hrtlab,
        unsigned int tid,
        unsigned int decimation,
        const char *path,
        enum si_datatype_t datatype,
        const void *addr,
        size_t n,
        const size_t dim[]
        )
{
    return reinterpret_cast<HRTLab::Main*>(hrtlab)->newSignal(
            tid, decimation, path, datatype,
            dim ? n : 1, dim ? dim : &n,
            reinterpret_cast<const char*>(addr)
            );
}

/////////////////////////////////////////////////////////////////////////////
int hrtlab_set_alias(
        struct hrtlab* hrtlab,
        const char *path,
        const char *alias)
{
    return reinterpret_cast<HRTLab::Main*>(hrtlab)->setAlias(
            path, alias);
}

/////////////////////////////////////////////////////////////////////////////
int hrtlab_set_unit(
        struct hrtlab* hrtlab,
        const char *path,
        const char *unit)
{
    return reinterpret_cast<HRTLab::Main*>(hrtlab)->setUnit(
            path, unit);
}

/////////////////////////////////////////////////////////////////////////////
int hrtlab_set_comment(
        struct hrtlab* hrtlab,
        const char *path,
        const char *comment)
{
    return reinterpret_cast<HRTLab::Main*>(hrtlab)->setComment(
            path, comment);
}

/////////////////////////////////////////////////////////////////////////////
int hrtlab_parameter(
        struct hrtlab* hrtlab,
        const char *path,
        unsigned int mode,
        enum si_datatype_t datatype,
        void *addr,
        size_t n,
        const size_t dim[],

        paramupdate_t paramcopy,
        void *priv_data
        )
{
    return reinterpret_cast<HRTLab::Main*>(hrtlab)->newParameter(
            path, mode, datatype, dim ? n : 1, dim ? dim : &n,
            reinterpret_cast<char*>(addr),
            paramcopy, priv_data);
}

/////////////////////////////////////////////////////////////////////////////
int hrtlab_start(struct hrtlab* hrtlab)
{
    return reinterpret_cast<HRTLab::Main*>(hrtlab)->start();
}

