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
                path, datatype, addr, tid, decimation,
                dim ? n : 1, dim ? dim : &n);

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
    Main *main = reinterpret_cast<Main*>(hrtlab);
    Parameter *p = main->newParameter( path, datatype, addr, mode, n, dim);
    main->setParameterTrigger(p, trigger, priv_data);

    return
        reinterpret_cast<struct variable *>(static_cast<HRTLab::Variable*>(p));
}

/////////////////////////////////////////////////////////////////////////////
void hrtlab_set_alias(struct hrtlab* hrtlab,
        struct variable *var, const char *alias)
{
    reinterpret_cast<Main*>(hrtlab)->setVariableAlias(
            reinterpret_cast<Variable*>(var), alias);
}

/////////////////////////////////////////////////////////////////////////////
void hrtlab_set_unit(struct hrtlab* hrtlab,
        struct variable *var, const char *unit)
{
    reinterpret_cast<Main*>(hrtlab)->setVariableUnit(
            reinterpret_cast<Variable*>(var), unit);
}

/////////////////////////////////////////////////////////////////////////////
void hrtlab_set_comment(struct hrtlab* hrtlab,
        struct variable *var, const char *comment)
{
    reinterpret_cast<Main*>(hrtlab)->setVariableComment(
            reinterpret_cast<Variable*>(var), comment);
}

/////////////////////////////////////////////////////////////////////////////
int hrtlab_init(struct hrtlab* hrtlab)
{
    return reinterpret_cast<Main*>(hrtlab)->init();
}
