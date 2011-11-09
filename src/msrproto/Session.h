/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdserv package.
 *
 *  pdserv is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pdserv is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pdserv. See COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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
#include "XmlParser.h"
#include "Outbuf.h"
#include "XmlDoc.h"

#include <cc++/socketport.h> 
#include <map>

namespace PdServ {
    class Parameter;
    class SessionTaskData;
}

namespace MsrXml {
    class Element;
}

namespace MsrProto {

class SubscriptionManager;
class Server;

class Session: public ost::SocketPort, public PdServ::Session {
    public:
        Session(
                Server *s,
                ost::SocketService *ss,
                ost::TCPSocket &socket,
                PdServ::Main *main);
        ~Session();

        void broadcast(Session *s, const MsrXml::Element &element);
        void parameterChanged(const PdServ::Parameter*,
                size_t startIndex, size_t nelem);
        void requestOutput();

        void processCommand();

        void requestSignal(const PdServ::Signal *s, bool state);

    private:
        Server * const server;

        typedef std::map<double, SubscriptionManager*> SubscriptionManagerMap;
        SubscriptionManagerMap subscriptionManager;

        // Reimplemented from SocketPort
        void expired();
        void pending();
        void output();
        void disconnect();

        // Reimplemented from PdServ::Session
        void newSignalList(const PdServ::Task *task,
                const PdServ::Signal * const *, size_t n);
        void newSignalData(const PdServ::SessionTaskData*);

        // Management variables
        bool writeAccess;
        bool quiet;
        bool echoOn;

        // <data> tag for the output stream
        MsrXml::Element dataTag;

        // Input and output buffering
        Outbuf outbuf;
        XmlParser inbuf;

        // Here are all the commands the MSR protocol supports
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
};

}
#endif //MSRSESSION_H
