/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef ETLSESSION
#define ETLSESSION

#include <cc++/socketport.h> 
#include <iosfwd>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <libxml/tree.h>

#include "rtlab/etl_data_info.h"

namespace HRTLab {
    class Main;
    class Signal;
    class Variable;
}

namespace EtlProto {

class Session: public ost::SocketPort, private std::streambuf {
    public:
        Session(
                ost::SocketService *ss,
                ost::TCPSocket &socket,
                HRTLab::Main *main);

    private:

        HRTLab::Main * const main;
        const std::vector<HRTLab::Signal*>& signals;

        std::string buf;
        std::string inbuf;
        const std::string crlf;

        // Map a signal to a set of subscription decimations
        std::vector<std::map<size_t, std::set<HRTLab::Signal*> > > subscribed;
        std::vector<std::map<size_t, bool> > dirty;
        std::map<HRTLab::Signal*, std::set<size_t> > signalDecimation;
        std::map<HRTLab::Variable*, bool> sent;
        std::vector<size_t> dataOffset;
        typedef std::map<unsigned int, unsigned int,
            std::greater<unsigned int> > DecimationMap;
        std::vector<DecimationMap> decimation;
        unsigned int * const signal_ptr_start;
        unsigned int *signal_ptr;

        static const char *getDTypeName(const enum si_datatype_t&);

        void printVariable(std::streambuf *, HRTLab::Variable*, const char*);
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
#endif //ETLSESSION
