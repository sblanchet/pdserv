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

#include "../Session.h"
#include "Inbuf.h"
#include "Outbuf.h"
#include "Attribute.h"

#include <cc++/socketport.h> 
#include <iosfwd>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <list>
#include <functional>

#include "rtlab/etl_data_info.h"

namespace HRTLab {
    class Signal;
    class Variable;
    class Parameter;
}

namespace MsrXml {
    class Element;
}

namespace MsrProto {

class Server;
class Task;

class Session: public ost::SocketPort, public HRTLab::Session {
    public:
        Session(
                Server *s,
                ost::SocketService *ss,
                ost::TCPSocket &socket,
                HRTLab::Main *main);
        ~Session();

        void broadcast(Session *s, const MsrXml::Element &element);
        void parameterChanged(const HRTLab::Parameter*);
        void requestOutput();

    private:

        Server * const server;

        // Reimplemented from SocketPort
        void expired();
        void pending();
        void output();
        void disconnect();

        // Reimplemented from HRTLab::Session
        std::string getName() const;
        std::string getClientName() const;
        size_t getCountIn() const;
        size_t getCountOut() const;
        struct timespec getLoginTime() const;
        size_t dataIn;
        size_t dataOut;
        struct timespec loginTime;

        bool writeAccess;
        bool quiet;
        bool echoOn;
        std::string remote;
        std::string applicationname;

        Outbuf outbuf;
        Inbuf inbuf;

        // Parser routines
        // Returning true within these routines will interrupt parsing
        // and wait for more characters
        void parseInput(char* &pos, const char *end);

        char *bptr, *pptr;
        enum ParseState {
            FindStart, GetCommand, GetToken, GetAttribute, GetQuote, GetValue,
        };
        ParseState parseState;
        const char *commandPtr, *attrName;
        char *attrValue;
        size_t commandLen, attrLen;
        char quote;
        Attr attr;

        void processCommand();

        void broadcast();
        void echo();
        void ping();
        void readChannel();
        void readParameter();
        void readParamValues();
        void readStatistics();
        void remoteHost();
        void writeParameter();
        void xsad();
        void xsod();

        std::vector<Task*> task;

        unsigned int * const signal_ptr_start;
        unsigned int *signal_ptr;
};

}
#endif //MSRSESSION_H
