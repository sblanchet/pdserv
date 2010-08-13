/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "Parameter.h"

//#include <iostream>
//using std::cout;
//using std::cerr;
//using std::endl;

using namespace HRTLab;

//////////////////////////////////////////////////////////////////////
Parameter::Parameter(
        unsigned int index,
        paramupdate_t paramupdate,
        void *priv_data,
        const char *path,
        const char *alias,
        enum si_datatype_t dtype,
        unsigned int ndims,
        const size_t dim[],
        const void *addr):
    Variable(index, path, alias, dtype, ndims, dim, 0, addr),
    paramupdate(paramupdate), priv_data(priv_data)
{
}
