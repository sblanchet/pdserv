/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef SESSIONSTATISTICS_H
#define SESSIONSTATISTICS_H

#include <ctime>
#include <string>

namespace HRTLab {

struct SessionStatistics {
    std::string remote;
    std::string client;
    size_t countIn;
    size_t countOut;
    struct timespec connectedTime;
};

}
#endif // SESSIONSTATISTICS_H
