/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "rtlab/rtlab.h"
#include "Main.h"
#include "Variable.h"
#include "Signal.h"
#include "Parameter.h"

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
    HRTLab::Variable *v =
        reinterpret_cast<HRTLab::Main*>(hrtlab)->newSignal(
                tid, decimation, path, datatype,
                dim ? n : 1, dim ? dim : &n,
                reinterpret_cast<const char*>(addr)
                );

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
        paramtrigger_t trigger = 0,
        void *priv_data = 0
        )
{
    HRTLab::Parameter *p =
        reinterpret_cast<HRTLab::Main*>(hrtlab)->newParameter(
                path, mode, datatype, addr, n, dim);
    p->setTrigger(trigger, priv_data);

    HRTLab::Variable *v = p;
    return reinterpret_cast<struct variable *>(v);
}

/////////////////////////////////////////////////////////////////////////////
void hrtlab_set_alias( void *var, const char *alias)
{
    reinterpret_cast<HRTLab::Variable*>(var)->setAlias( alias);
}

/////////////////////////////////////////////////////////////////////////////
void hrtlab_set_unit( void *var, const char *unit)
{
    reinterpret_cast<HRTLab::Variable*>(var)->setUnit(unit);
}

/////////////////////////////////////////////////////////////////////////////
void hrtlab_set_comment( void *var, const char *comment)
{
    reinterpret_cast<HRTLab::Variable*>(var)->setComment(comment);
}

/////////////////////////////////////////////////////////////////////////////
int hrtlab_start(struct hrtlab* hrtlab)
{
    return reinterpret_cast<HRTLab::Main*>(hrtlab)->start();
}

