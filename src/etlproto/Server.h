/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef ETLSERVER_H
#define ETLSERVER_H

#include <cc++/thread.h>

namespace HRTLab {
    class Main;
}

namespace EtlProto {

class Server:public ost::Thread {
    public:
        Server(HRTLab::Main *main);
        ~Server();

    private:

        HRTLab::Main * const main;

        void run();

};

}
#endif //ETLSERVER_H
