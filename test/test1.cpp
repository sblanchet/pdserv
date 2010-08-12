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

uint16_t var1[2][3][4];

int copy_param(void *priv_data, const void* src, size_t len)
{
    memcpy(priv_data, src, len);
    return 0;
}

int main(int argc, const char *argv[])
{
    struct hrtlab *hrtlab = hrtlab_init(argc, argv, argv[0], "1.0", 1.0, 1, 0);
    unsigned int var1_dims[] = {2,3,4};

    assert(!hrtlab_signal(hrtlab, 0, 1, "/path/to/variable", "var1",
            si_uint16_T, 3, var1_dims, &var1));

    assert(!hrtlab_parameter(hrtlab, 0, &var1, "/path\"/to/püaram", "vüar1",
            si_uint16_T, 3, var1_dims, &var1));

    assert(!hrtlab_start(hrtlab));

    // Need to be root
    //assert(!mlockall(MCL_CURRENT|MCL_FUTURE));

    while (1) {
        sleep(1);
        hrtlab_update(hrtlab, 0);
    }

    hrtlab_exit(hrtlab);

    return 0;
}
