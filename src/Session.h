/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef SESSION_H
#define SESSION_H

#include <string>
#include <ctime>
#include <vector>

#include "SessionStatistics.h"

namespace HRTLab {

class Task;
class Signal;
class Main;
class Receiver;

class Session {
    public:
        Session(Main *main);
        ~Session();

        SessionStatistics getStatistics() const;

        const Receiver& rxPdo();

    protected:
        Main * const main;
        Receiver * const receiver;

        size_t inBytes;
        size_t outBytes;
        std::string remoteHost;
        std::string client;

    private:

        struct timespec connectedTime;
};

}
#endif //SESSION_H
