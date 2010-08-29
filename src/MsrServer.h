/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef MSRSERVER_H
#define MSRSERVER_H

#include <cc++/thread.h>

namespace HRTLab {
    class Main;
}

namespace MsrProto {

class Session;

class Server:public ost::Thread {
    public:
        Server(HRTLab::Main *main);
        ~Server();

        void broadcast(Session *s,
                const std::string *action, const std::string *text);

    private:

        HRTLab::Main * const main;

        void run();

};

}
#endif //MSRSERVER_H
