#include "pdserv/pdserv.h"
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

int copy_param(struct pdtask *, const struct variable *,
        void* dst, const void* src, size_t len, void *priv_data)
{
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
    struct pdserv *pdserv =
        pdserv_create(argc, argv, argv[0], "1.0", gettime);
    unsigned int var1_dims[] = {2,3,4};
    struct timespec time;
    double dbl[3] = { 13,14,15};
    double dbltime;
    double tick = 10;
    uint32_t param[] = {1,2,3,4};
    struct pdtask *task[3];

    task[0] = pdserv_create_task(pdserv, 0.1, "Task1");

//    assert(!pdserv_signal(pdserv, 0, 1, "/path/to/variable", "var1",
//            si_uint16_T, 3, var1_dims, var1));

    assert(pdserv_signal(task[0], 1, "/path/to/double",
            si_double_T, dbl, 3, NULL));

    assert(pdserv_signal(task[0], 1, "/Tme",
            si_double_T, &dbltime, 1, NULL));

    assert(pdserv_signal(task[0], 1, "/path/to/var2",
            si_uint16_T, var1, 3, var1_dims));

    struct variable *p3 = pdserv_parameter(pdserv, "/path/to/mdimparam", 0666,
            si_uint16_T, var1, 3, var1_dims, copy_param, (void*)10);
    assert(p3);

    struct variable *p1 = pdserv_parameter(pdserv,
            "/Taskinfo/Abtastfrequenz", 0666, si_double_T, &tick, 1, 0, 0, 0);
    assert(p1);

    struct variable *p = pdserv_parameter(pdserv, "/path/to/param", 0666,
            si_uint32_T, param, 4, 0, copy_param, (void*)10);
    assert(p);

    assert(!pdserv_prepare(pdserv));

    // Need to be root
    //assert(!mlockall(MCL_CURRENT|MCL_FUTURE));

    int i;
    while (1) {
        //sleep(1);
        usleep(100000);
        clock_gettime(CLOCK_REALTIME, &time);

        pdserv_get_parameters(pdserv, task[0], &time);
        var1[1][0][0] = i++;
        dbl[0] += param[0];
        dbl[2] += param[2];
        var1[5][0][0] += param[1];
        dbltime = time.tv_sec + time.tv_nsec * 1.0e-9;
        pdserv_update(task[0], &time, 0, 0);
    }

    pdserv_exit(pdserv);

    return 0;
}
