/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef SESSION_H
#define SESSION_H

#include <string>
#include <ctime>
#include <vector>

#include "PdoSignalList.h"

namespace HRTLab {

class Main;
class PdoSignalList;

class Session {
    public:
        Session(Main *main);
        virtual ~Session();

        virtual std::string getName() const;
        virtual std::string getClientName() const;
        virtual size_t getCountIn() const;
        virtual size_t getCountOut() const;
        virtual struct timespec getLoginTime() const;

        virtual void newSignalMap(unsigned int tid,
                const HRTLab::PdoSignalList::SigOffsetMap &s);
        virtual void newPdoData(unsigned int tid, unsigned int seqNo,
                const struct timespec *t, const char *);

        bool isSignalActive(const Signal *) const;

    protected:
        Main * const main;

        void receivePdo();

    private:
        void * const *pdoBlockPtr;

        std::vector<PdoSignalList> pdoSignalList;
};

}
#endif //SESSION_H
