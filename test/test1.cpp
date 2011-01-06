#include "pdcomserv/pdcomserv.h"
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

int copy_param(unsigned int tid, char checkOnly,
        void* dst, const void* src, size_t len, void *priv_data)
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
    struct pdcomserv *pdcomserv =
        pdcomserv_create(argc, argv, argv[0], "1.0", 0.1, 1, 0, gettime);
    unsigned int var1_dims[] = {2,3,4};
    struct timespec time;
    double dbl[3] = { 13,14,15};
    double dbltime;
    double tick = 10;
    uint32_t param[] = {1,2,3,4};

//    assert(!pdcomserv_signal(pdcomserv, 0, 1, "/path/to/variable", "var1",
//            si_uint16_T, 3, var1_dims, var1));

    assert(pdcomserv_signal(pdcomserv, 0, 1, "/path/to/double",
            si_double_T, dbl, 3, NULL));

    assert(pdcomserv_signal(pdcomserv, 0, 1, "/Time",
            si_double_T, &dbltime, 1, NULL));

    assert(pdcomserv_signal(pdcomserv, 0, 1, "/path/to/var2",
            si_uint16_T, var1, 3, var1_dims));

    struct variable *p1 = pdcomserv_parameter(pdcomserv,
            "/Taskinfo/Abtastfrequenz", 0666, si_double_T, &tick, 1, 0, 0, 0);
    struct variable *p = pdcomserv_parameter(pdcomserv, "/path/to/param", 0666,
            si_uint32_T, param, 4, 0, copy_param, (void*)10);
    assert(p);

    assert(!pdcomserv_init(pdcomserv));

    // Need to be root
    //assert(!mlockall(MCL_CURRENT|MCL_FUTURE));

    int i;
    while (1) {
        //sleep(1);
        usleep(100000);
        var1[1][0][0] = i++;
        dbl[0] += param[0];
        dbl[2] += param[2];
        var1[5][0][0] += param[1];
        clock_gettime(CLOCK_REALTIME, &time);
        dbltime = time.tv_sec + time.tv_nsec * 1.0e-9;
        pdcomserv_update(pdcomserv, 0, &time);
    }

    pdcomserv_exit(pdcomserv);

    return 0;
}
