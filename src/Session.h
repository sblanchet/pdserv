/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef SESSION_H
#define SESSION_H

#include <string>
#include <ctime>
#include <vector>

namespace HRTLab {

class Main;
class Task;
class Variable;

class Session {
    public:
        Session(Main *main);
        virtual ~Session();

        virtual std::string getName() const;
        virtual std::string getClientName() const;
        virtual size_t getCountIn() const;
        virtual size_t getCountOut() const;
        virtual struct timespec getLoginTime() const;

        virtual void newVariableList(const Task *, const Variable **,
                size_t n);
        virtual void newPdoData(const Task *, unsigned int seqNo,
                const struct timespec *t, const char *);

    protected:
        Main * const main;
};

}
#endif //SESSION_H
