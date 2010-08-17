/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef TCPSERVER
#define TCPSERVER

#include <cc++/socketport.h> 
#include <iosfwd>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <libxml/tree.h>


namespace HRTLab {

class Main;
class Signal;

class TCPServer: public ost::SocketPort, private std::streambuf {
    public:
        TCPServer(
                ost::SocketService *ss,
                ost::TCPSocket &socket,
                Main *main);

    private:

        Main * const main;
        const std::vector<Signal*>& signals;

        std::string buf;
        std::string inbuf;
        const std::string crlf;

        // Map a signal to a set of subscription decimations
        std::vector<std::map<size_t, std::set<Signal*> > > subscribed;
        std::map<Signal*, std::set<size_t> > signalDecimation;
        std::vector<size_t> dataOffset;
        typedef std::map<unsigned int, unsigned int,
            std::greater<unsigned int> > DecimationMap;
        std::vector<DecimationMap> decimation;
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
