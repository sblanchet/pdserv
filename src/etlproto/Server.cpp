#include "Server.h"
#include <cc++/socketport.h>

#include "Session.h"

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

using namespace EtlProto;

/////////////////////////////////////////////////////////////////////////////
Server::Server(PdServ::Main *main): main(main)
{
}

/////////////////////////////////////////////////////////////////////////////
Server::~Server()
{
}

/////////////////////////////////////////////////////////////////////////////
void Server::run()
{
    cout << "#################" << endl;
    ost::SocketService svc;
    ost::TCPSocket socket(ost::IPV4Address("0.0.0.0"), 4000);
    while (true) {
        new Session(&svc, socket, main);
    }
}
