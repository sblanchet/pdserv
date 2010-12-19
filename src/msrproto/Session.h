/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef MSRSESSION_H
#define MSRSESSION_H

// Version der MSR_LIBRARIES 
#define _VERSION  6
#define _PATCHLEVEL 0
#define _SUBLEVEL  10


#define MSR_VERSION (((_VERSION) << 16) + ((_PATCHLEVEL) << 8) + (_SUBLEVEL))

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
#include "XmlDoc.h"
#include "SubscriptionChange.h"

#include <cc++/socketport.h> 
#include <string>
#include <map>
#include <set>

#include "rtlab/etl_data_info.h"

namespace HRTLab {
    class Variable;
    class Parameter;
    class Variable;
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
        void parametersChanged(const HRTLab::Parameter* const *, size_t n);
        void requestOutput();

        void processCommand(const char *command, const Attr &attr);

        size_t getVariableIndex(const HRTLab::Variable *v) const;

    private:
        Server * const server;
        Task ** const task;

        std::map<const void *, size_t> variableIndexMap;

        SubscriptionChange subscriptionChange;

        size_t signalCount;
        const HRTLab::Signal **signal;

        size_t parameterCount;
        const HRTLab::Parameter **parameter;

        // Reimplemented from SocketPort
        void expired();
        void pending();
        void output();
        void disconnect();

        // Reimplemented from HRTLab::Session
        void newSignalList(const HRTLab::Task *task,
                const HRTLab::Signal * const *, size_t n);
        void newSignalData(const HRTLab::Receiver&);

        // Management variables
        bool writeAccess;
        bool quiet;
        bool echoOn;

        // <data> tag for the output stream
        MsrXml::Element dataTag;

        // Input and output buffering
        Outbuf outbuf;
        Inbuf inbuf;

        // Here are all the commands the MSR protocol supports
        void broadcast(const Attr &);
        void echo(const Attr &);
        void ping(const Attr &);
        void readChannel(const Attr &);
        void readParameter(const Attr &);
        void readParamValues(const Attr &);
        void readStatistics(const Attr &);
        void remoteHost(const Attr &);
        void writeParameter(const Attr &);
        void xsad(const Attr &);
        void xsod(const Attr &);

};

}
#endif //MSRSESSION_H
