/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef TCPSERVER
#define TCPSERVER

#include <cc++/socketport.h> 
#include <iosfwd>
#include <string>
#include <vector>
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

        Main * const main;

        std::string buf;
        std::string inbuf;
        const std::string crlf;

        std::vector<std::vector<unsigned int> > subscribers;
        unsigned int * const signal_ptr_start;
        unsigned int *signal_ptr;

        enum ParseState_t {
            Idle,
        } state;
        ParseState_t ParseInstruction(const std::string&);
        void list();
        void subscribe(const std::string& path, unsigned int decimation);

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
