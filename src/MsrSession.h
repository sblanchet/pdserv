/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef MSRSESSION_H
#define MSRSESSION_H

// Version der MSR_LIBRARIES 
#define VERSION  6
#define PATCHLEVEL 0
#define SUBLEVEL  10


#define MSR_VERSION (((VERSION) << 16) + ((PATCHLEVEL) << 8) + (SUBLEVEL))

//Liste der Features der aktuellen rtlib-Version, wichtig, muß aktuell gehalten werden
//da der Testmanager sich auf die Features verläßt

#define MSR_FEATURES "pushparameters,binparameters,maschinehalt,eventchannels,statistics,pmtime,aic,messages"

/* pushparameters: Parameter werden vom Echtzeitprozess an den Userprozess gesendet bei Änderung
   binparameters: Parameter können Binär übertragen werden
   maschinehalt: Rechner kann durch Haltbefehl heruntergefahren werden
   eventchannels: Kanäle werden nur bei Änderung übertragen
   statistics: Statistische Informationen über die anderen verbundenen Echtzeitprozesses
   pmtime: Die Zeit der Änderungen eines Parameters wird vermerkt und übertragen
   aic: ansychrone Input Kanäle
   messages: Kanäle mit dem Attribut: "messagetyp" werden von der msr_lib überwacht und bei Änderung des Wertes als Klartextmeldung verschickt V:6.0.10

*/

#include <cc++/socketport.h> 
#include <iosfwd>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <functional>

#include "rtlab/etl_data_info.h"

namespace HRTLab {
    class Main;
    class Signal;
    class Variable;
    class Parameter;
}

namespace MsrXml {
    class Element;
}

namespace MsrProto {

class Server;

class Session: public ost::SocketPort,
    private std::streambuf, private std::ostream {
    public:
        Session(
                Server *s,
                ost::SocketService *ss,
                ost::TCPSocket &socket,
                HRTLab::Main *main);
        ~Session();

        void broadcast(Session *s, const struct timespec &t,
                const std::string *action, const std::string *text);

        void parameterChanged(const HRTLab::Parameter*);

    private:

        HRTLab::Main * const main;
        Server * const server;

        bool writeAccess;

        std::string buf;
        std::string inbuf;

//        // Map a signal to a set of subscription decimations
//        std::vector<std::map<size_t, std::set<HRTLab::Signal*> > > subscribed;
//        std::vector<std::map<size_t, bool> > dirty;
//        std::map<HRTLab::Signal*, std::set<size_t> > signalDecimation;
//        std::map<HRTLab::Variable*, bool> sent;
//        std::vector<size_t> dataOffset;
//        typedef std::map<unsigned int, unsigned int,
//            std::greater<unsigned int> > DecimationMap;
//        std::vector<DecimationMap> decimation;
        unsigned int * const signal_ptr_start;
        unsigned int *signal_ptr;
//
        static const char *getDTypeName(const HRTLab::Variable*);
//
//        void printVariable(std::streambuf *, HRTLab::Variable*, const char*);
//        enum ParseState_t {
//            Idle,
//        } state;
//        ParseState_t ParseInstruction(const std::string&);
//        void list();
//        void subscribe(const std::string& path, unsigned int decimation);
//
//        xmlChar *xmlchar;
//        int xmlcharlen;
//        xmlChar *utf8(const std::string& s);
//        xmlCharEncodingHandlerPtr encoding;

        // Parser routines
        // Returning true within these routines will interrupt parsing
        // and wait for more characters
        bool search(char, const char* &pos, const char *end);
        bool evalExpression(const char* &pos, const char *end);
        bool evalIdentifier(const char* &pos, const char *end);
        bool evalAttribute(const char* &pos, const char *end,
                std::string &name, std::string &value);
        bool evalReadParameter(const char* &pos, const char *end);

        typedef std::map<const std::string, std::string> AttributeMap;
        typedef void (Session::*CmdFunc)(const AttributeMap&);
        typedef std::map<const std::string, CmdFunc> CommandMap;
        static CommandMap commandMap;
        void pingCmd(const AttributeMap &attributes);
        void readParameterCmd(const AttributeMap &attributes);
        void readParamValuesCmd(const AttributeMap &attributes);
        void writeParameterCmd(const AttributeMap &attributes);
        void readChannelsCmd(const AttributeMap &attributes);
        void xsadCmd(const AttributeMap &attributes);
        void xsodCmd(const AttributeMap &attributes);
        void remoteHostCmd(const AttributeMap &attributes);
        void broadcastCmd(const AttributeMap &attributes);
        void readStatisticsCmd(const AttributeMap &attributes);
        void nullCmd(const AttributeMap &attributes);

        std::set<HRTLab::Signal*> readChannel;

        static std::string toCSV(
                const HRTLab::Variable *v, const char* data);
        static std::string toHexDec(
                const HRTLab::Variable *v, const char* data);
        void setParameterAttributes(MsrXml::Element *e,
                const HRTLab::Parameter *p, bool shortReply, bool hex);
        void setChannelAttributes(MsrXml::Element *e,
                const HRTLab::Signal *s, bool shortReply, const char *data);

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
#endif //MSRSESSION_H
