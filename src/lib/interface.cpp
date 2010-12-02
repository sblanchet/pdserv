/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "Main.h"
#include "Signal.h"
#include "Parameter.h"

/////////////////////////////////////////////////////////////////////////////
struct hrtlab* hrtlab_create(int argc, const char *argv[], 
        const char *name, const char *version, double baserate,
        unsigned int nst, const unsigned int decimation[],
        int (*gettime)(struct timespec*))
{
    return reinterpret_cast<struct hrtlab*>(
            new Main(argc, argv, name, version, baserate, nst,
                decimation, gettime));
}

/////////////////////////////////////////////////////////////////////////////
void hrtlab_exit(struct hrtlab* hrtlab)
{
    delete reinterpret_cast<Main*>(hrtlab);
}

/////////////////////////////////////////////////////////////////////////////
void hrtlab_update(struct hrtlab* hrtlab, int st, const struct timespec *t)
{
    reinterpret_cast<Main*>(hrtlab)->update(st, t);
}

/////////////////////////////////////////////////////////////////////////////
struct variable *hrtlab_signal(
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
    HRTLab::Variable *v = reinterpret_cast<Main*>(hrtlab)->newSignal(
                path, datatype, addr, tid, decimation, n, dim);

    return reinterpret_cast<struct variable *>(v);
}

/////////////////////////////////////////////////////////////////////////////
struct variable *hrtlab_parameter(
        struct hrtlab* hrtlab,
        const char *path,
        unsigned int mode,
        enum si_datatype_t datatype,
        void *addr,
        size_t n,
        const size_t dim[],
        paramtrigger_t _trigger = 0,
        void *_priv_data = 0
        )
{
    Main *main = reinterpret_cast<Main*>(hrtlab);
    Parameter *p = main->newParameter( path, datatype, addr, mode, n, dim);
    p->trigger = _trigger;
    p->priv_data = _priv_data;

    return
        reinterpret_cast<struct variable *>(static_cast<HRTLab::Variable*>(p));
}

/////////////////////////////////////////////////////////////////////////////
void hrtlab_set_alias( struct variable *var, const char *_alias)
{
    reinterpret_cast<HRTLab::Variable*>(var)->alias = _alias;
}

/////////////////////////////////////////////////////////////////////////////
void hrtlab_set_unit( struct variable *var, const char *_unit)
{
    reinterpret_cast<HRTLab::Variable*>(var)->unit = _unit;
}

/////////////////////////////////////////////////////////////////////////////
void hrtlab_set_comment( struct variable *var, const char *_comment)
{
    reinterpret_cast<HRTLab::Variable*>(var)->comment = _comment;
}

/////////////////////////////////////////////////////////////////////////////
int hrtlab_init(struct hrtlab* hrtlab)
{
    return reinterpret_cast<Main*>(hrtlab)->init();
}
