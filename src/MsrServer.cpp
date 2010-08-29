#include "MsrServer.h"
#include "Main.h"
#include <cc++/socketport.h>

#include "MsrSession.h"

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Server::Server(HRTLab::Main *main): main(main)
{
}

/////////////////////////////////////////////////////////////////////////////
Server::~Server()
{
}

/////////////////////////////////////////////////////////////////////////////
void Server::run()
{
    ost::SocketService svc;
    ost::TCPSocket socket(ost::IPV4Address("0.0.0.0"), 2345);
    while (true) {
        new Session(this, &svc, socket, main);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Server::broadcast(Session *s,
        const std::string *action, const std::string *text)
{
    struct timespec ts;
    main->gettime(&ts);

    s->broadcast(s, ts, action, text);
}
