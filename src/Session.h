/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef SESSION_H
#define SESSION_H

#include <string>
#include <ctime>
#include <vector>

namespace HRTLab {

class Task;
class Variable;
class Main;

class Session {
    public:
        Session(Main *main);
        virtual ~Session();

        virtual void newVariableList(const Task *, const Variable **,
                size_t n);
        virtual void newPdoData(const Task *, unsigned int seqNo,
                const struct timespec *t, const char *);

        typedef struct {
            std::string remote;
            std::string client;
            size_t countIn;
            size_t countOut;
            struct timespec connectedTime;
        } Statistics;
        Statistics getStatistics() const;

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
