#include "rtlab/rtlab.h"
#include <stdint.h>

uint16_t var1[2][3][4];

int main(int argc, const char *argv[])
{
    struct hrtlab *hrtlab = hrtlab_init(argc, argv, 1);
    unsigned int var1_dims[] = {2,3,4};

    hrtlab_signal(hrtlab, 1, 1, "/path/to/variable", "var1",
            si_uint16_T, 3, var1_dims, &var1);
    return 0;
}
