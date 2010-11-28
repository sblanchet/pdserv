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

class Session {
    public:
        Session(Main *main);
        virtual ~Session();

        virtual void newSignalList(unsigned int tid, const Signal * const *,
                size_t n) = 0;
        virtual void newPdoData(unsigned int tid, unsigned int seqNo,
                const struct timespec *t) = 0;

        SessionStatistics getStatistics() const;

    protected:
        Main * const main;

        size_t inBytes;
        size_t outBytes;
        std::string remoteHost;
        std::string client;

    private:
        struct timespec connectedTime;
};

}
#endif //SESSION_H
