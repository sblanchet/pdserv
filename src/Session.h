/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef SESSION_H
#define SESSION_H

#include <string>
#include <ctime>

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

        // These methods are called from within the context of rxPdo
        virtual void newSignalList(unsigned int tid,
                const Signal * const *, size_t n) = 0;
        virtual void newSignalData(const Receiver&) = 0;

        void rxPdo();

    protected:
        Main * const main;

        size_t inBytes;
        size_t outBytes;
        std::string remoteHost;
        std::string client;

    private:
        Receiver ** const receiver;

        struct timespec connectedTime;
};

}
#endif //SESSION_H
