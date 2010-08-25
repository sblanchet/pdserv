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

class Server:public ost::Thread {
    public:
        Server(HRTLab::Main *main);
        ~Server();

    private:

        HRTLab::Main * const main;

        void run();

};

}
#endif //MSRSERVER_H
