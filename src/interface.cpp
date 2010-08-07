#include "rtlab/rtlab.h"
#include "Main.h"

struct rtlab* rtlab_init(int argc, const char *argv[], int nst)
{
    return reinterpret_cast<struct rtlab*>(new RTLab::Main(argc, argv, nst));
}

void rtlab_exit(struct rtlab* rtlab)
{
    delete reinterpret_cast<RTLab::Main*>(rtlab);
}

void rtlab_update(struct rtlab* rtlab, int st)
{
    reinterpret_cast<RTLab::Main*>(rtlab)->update(st);
}
