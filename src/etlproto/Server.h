/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef ETLSERVER_H
#define ETLSERVER_H

#include <cc++/thread.h>

namespace PdServ {
    class Main;
}

namespace EtlProto {

class Server:public ost::Thread {
    public:
        Server(PdServ::Main *main);
        ~Server();

    private:

        PdServ::Main * const main;

        void run();

};

}
#endif //ETLSERVER_H
