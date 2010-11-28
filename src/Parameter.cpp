/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "config.h"

#include <ctime>
#include <algorithm>

#include "Parameter.h"
#include "Main.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace HRTLab;

//////////////////////////////////////////////////////////////////////
Parameter::Parameter( Main *main,
        const char *path,
        unsigned int mode,
        enum si_datatype_t dtype,
        unsigned int ndims,
        const size_t *dim):
    Variable(path, dtype, ndims, dim),
    mode(mode), main(main)
{
}
