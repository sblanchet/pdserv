/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef SESSION_H
#define SESSION_H

#include <string>
#include <ctime>
#include <vector>

#include "Main.h"

namespace HRTLab {

class Task;
class Variable;

class Session {
    public:
        Session(Main *main);
        virtual ~Session();

        virtual void newVariableList(const Task *, const Variable **,
                size_t n);
        virtual void newPdoData(const Task *, unsigned int seqNo,
                const struct timespec *t, const char *);
        virtual void getSessionStatistics(Main::SessionStatistics&) const;

    protected:
        Main * const main;
};

}
#endif //SESSION_H
