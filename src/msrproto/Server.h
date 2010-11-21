/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef MSRSERVER_H
#define MSRSERVER_H

#include <set>
#include <cc++/thread.h>

#include "Session.h"

namespace HRTLab {
    class Main;
    class Parameter;
}

namespace MsrXml {
    class Element;
}

namespace MsrProto {

class Session;

class Server:public ost::Thread {
    public:
        Server(HRTLab::Main *main);
        ~Server();

        void broadcast(Session *s, const MsrXml::Element&);

        void parameterChanged(const HRTLab::Parameter**, size_t n);

        void sessionClosed(Session *s);

        void getSessionStatistics(std::list<Session::Statistics>& stats) const;

    private:

        HRTLab::Main * const main;
        std::set<Session*> sessions;

        mutable ost::Semaphore mutex;

        void run();

};

}
#endif //MSRSERVER_H
