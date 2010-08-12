/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef TCPSERVER
#define TCPSERVER

#include <cc++/socketport.h> 
#include <iosfwd>
#include <string>
#include <libxml/tree.h>


namespace HRTLab {

class Main;

class TCPServer: public ost::SocketPort, private std::streambuf {
    public:
        TCPServer(
                ost::SocketService *ss,
                ost::TCPSocket &socket,
                Main *main);

    private:

        const Main *main;

        std::string buf;
        std::string inbuf;
        const std::string crlf;

        enum Instruction_value {
            LS,
        };
        void ParseInstruction(std::istream&);
        void list();

        xmlChar *xmlchar;
        int xmlcharlen;
        xmlChar *utf8(const std::string& s);
        xmlCharEncodingHandlerPtr encoding;

        // Reimplemented from streambuf
        int sync();
        int overflow(int c);
        std::streamsize xsputn ( const char * s, std::streamsize n );

        // Reimplemented from SocketPort
        void expired();
        void pending();
        void output();
        void disconnect();
};

}
#endif //TCPSERVER
