#include "rtlab/rtlab.h"
#include <stdint.h>
#include <cstring>
#include <stdio.h>
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

int copy_param(unsigned int tid, char checkOnly, void* dst, const void* src, size_t len,
        void *priv_data)
{
    printf("copy_param checking %i\n", checkOnly);
    if (checkOnly)
        return 0;

    memcpy(dst, src, len);
    printf("hallo %p\n", priv_data);
    return 0;
}

int gettime(struct timespec *t)
{
    return clock_gettime(CLOCK_REALTIME, t);
}

int main(int argc, const char *argv[])
{
    struct hrtlab *hrtlab = hrtlab_create(argc, argv, argv[0], "1.0", 0.1, 1, 0,
            gettime);
    unsigned int var1_dims[] = {2,3,4};
    struct timespec time;
    double dbl[3] = { 13,14,15};
    uint32_t param[] = {1,2,3,4};

//    assert(!hrtlab_signal(hrtlab, 0, 1, "/path/to/variable", "var1",
//            si_uint16_T, 3, var1_dims, var1));

    assert(hrtlab_signal(hrtlab, 0, 1, "/path/to/var2",
            si_uint16_T, var1+5, 1, NULL));

    assert(hrtlab_signal(hrtlab, 0, 1, "/path/to/double",
            si_double_T, dbl, 1, NULL));

    struct variable *p = hrtlab_parameter(hrtlab, "/path/to/param", 0666,
            si_uint32_T, param, 4, 0, copy_param, (void*)10);
    assert(p);

    assert(!hrtlab_init(hrtlab));

    // Need to be root
    //assert(!mlockall(MCL_CURRENT|MCL_FUTURE));

    int i;
    while (1) {
        //sleep(1);
        usleep(100000);
        var1[1][0][0] = i++;
        dbl[0] += param[0];
        var1[5][0][0] += param[1];
        clock_gettime(CLOCK_REALTIME, &time);
        hrtlab_update(hrtlab, 0, &time);
    }

    hrtlab_exit(hrtlab);

    return 0;
}
