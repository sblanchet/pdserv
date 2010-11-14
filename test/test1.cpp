#include "rtlab/rtlab.h"
#include <stdint.h>
#include <cstring>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

uint16_t var1[2][3][4] = {
    { {1,2,3,4}, {5,6,7}, },
};

int copy_param(void *priv_data, void* dst, const void* src, size_t nelem,
        enum si_datatype_t datatype, size_t ndims, const size_t dim[])
{
    memcpy(priv_data, src, nelem);
    return 0;
}

int gettime(struct timespec *t)
{
    return clock_gettime(CLOCK_REALTIME, t);
}

int main(int argc, const char *argv[])
{
    struct hrtlab *hrtlab = hrtlab_init(argc, argv, argv[0], "1.0", 1.0, 1, 0,
            gettime);
    unsigned int var1_dims[] = {2,3,4};
    struct timespec time;
    double dbl[3] = { 13,14,15};

//    assert(!hrtlab_signal(hrtlab, 0, 1, "/path/to/variable", "var1",
//            si_uint16_T, 3, var1_dims, var1));

    assert(!hrtlab_signal(hrtlab, 0, 1, "/path/to/var2", "var1",
            si_uint16_T, 1, NULL, var1+5));

    assert(!hrtlab_signal(hrtlab, 0, 1, "/path/to/double", "var1",
            si_double_T, 1, NULL, dbl));

    assert(!hrtlab_parameter(hrtlab,
                "/path\"/to/püaram", "vüar1",
                si_uint16_T, 3, var1_dims, var1, 0, 0, 0));

    assert(!hrtlab_start(hrtlab));

    // Need to be root
    //assert(!mlockall(MCL_CURRENT|MCL_FUTURE));

    int i;
    while (1) {
        //sleep(1);
        usleep(100000);
        var1[1][0][0] = i++;
        dbl[0] += 1;
        var1[5][0][0] += 1;
        clock_gettime(CLOCK_REALTIME, &time);
        hrtlab_update(hrtlab, 0, &time);
    }

    hrtlab_exit(hrtlab);

    return 0;
}
