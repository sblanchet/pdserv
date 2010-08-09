/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "TCPServer.h"
#include "Main.h"

#include <iostream>
#include <iomanip>
using std::cout;
using std::cerr;
using std::endl;

using namespace HRTLab;

/////////////////////////////////////////////////////////////////////////////
TCPServer::TCPServer( ost::SocketService *ss,
        ost::TCPSocket &socket, Main *main):
    SocketPort(0, socket), main(main), crlf("\r\n")
{
    std::ostream s(this);
    setCompletion(false);
    setTimer(1000);
    sputn("Welcome\n", 8);
    s << "Welcome" << crlf
        << "Name: " << main->name << crlf
        << "Version: " << main->version << crlf
        << "Sampletime: " << main->baserate << crlf
        << "NumSt: "  << main->nst << crlf
        << "Decimations: ";
    for (unsigned int i = 0; i < main->nst - 1; i++) {
        if (i)
            s << ",";
        s << main->decimation[i];
    }
    s << crlf << std::flush;

    attach(ss);
}

/////////////////////////////////////////////////////////////////////////////
int TCPServer::sync()
{
    setDetectOutput(true);
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
int TCPServer::overflow(int c)
{
    buf.append(1,c);
    return c;
}

/////////////////////////////////////////////////////////////////////////////
std::streamsize TCPServer::xsputn ( const char * s, std::streamsize n )
{
    buf.append(s,n);
    return n;
}

/////////////////////////////////////////////////////////////////////////////
void TCPServer::expired()
{
    cout << __func__ << endl;
    setTimer(1000);
}

/////////////////////////////////////////////////////////////////////////////
void TCPServer::pending()
{
    cout << __func__ << endl;
    char buf[1024];

    ssize_t n = receive(buf, sizeof(buf));
    if (n <= 0) {
        delete this;
        return;
    }

    sputn(buf, n);
}

/////////////////////////////////////////////////////////////////////////////
void TCPServer::output()
{
    cout << __func__ << endl;
    ssize_t n = send(buf.c_str(), buf.length());
    if (n > 0)
        buf.erase(0,n);
    else {
        delete this;
        return;
    }

    if (buf.empty())
        setDetectOutput(false);
}

/////////////////////////////////////////////////////////////////////////////
void TCPServer::disconnect()
{
    cout << __func__ << endl;
    delete this;
}
