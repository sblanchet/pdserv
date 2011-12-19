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
#include "XmlElement.h"

#include <cc++/thread.h> 
#include <cc++/socketport.h> 
#include <map>
#include <set>
#include <cstdio>

namespace PdServ {
    class Parameter;
    class SessionTaskData;
    class Task;
    class TaskStatistics;
}

namespace MsrProto {

class SubscriptionManager;
class TaskStatistics;
class SessionStatistics;
class Server;
class XmlElement;
class Subscription;
class VariableDirectory;

class Session: public ost::Thread, public PdServ::Session {
    public:
        Session( Server *s,
                ost::TCPSocket *socket);
        ~Session();

        void broadcast(Session *s, const std::string &element);
        void parameterChanged(const PdServ::Parameter*,
                size_t startIndex, size_t nelem);
        void setAIC(const Parameter* p);
        void getSessionStatistics(PdServ::SessionStatistics &stats) const;

        void processCommand();

        void requestSignal(const PdServ::Signal *s, bool state);

        uint32_t *getUInt32() const {
            return &tmp.uint32;
        }

        double *getDouble() const {
            return &tmp.dbl;
        }

    private:
        Server * const server;
        const VariableDirectory &root;

        struct TCPStream: public ost::Socket, public std::streambuf {
            TCPStream(ost::TCPSocket *socket);

            int read(char *buf, size_t n, timeout_t timeout);
            int getSocket() const;

            // Reimplemented from std::streambuf
            int overflow ( int c );
            std::streamsize xsputn ( const char * s, std::streamsize n );
            int sync();

            size_t inBytes;
            size_t outBytes;

            ost::IPV4Host peer;
            ost::tpport_t port;

            FILE *file;
        };
        TCPStream tcp;

        std::ostream ostream;

        ost::Semaphore mutex;
        size_t aicDelay;
        typedef std::set<const PdServ::Parameter*> AicSet;
        AicSet aic;

        typedef std::map<const PdServ::Task*, SubscriptionManager*>
            SubscriptionManagerMap;
        SubscriptionManagerMap subscriptionManager;

        // Temporary memory space needed to handle statistic channels
        union {
            uint32_t uint32;
            double dbl;
        } mutable tmp;

        // Reimplemented from ost::Thread
        void run();

        // Reimplemented from PdServ::Session
        void newSignal(const PdServ::Task *task, const PdServ::Signal *);
        void newSignalData(const PdServ::SessionTaskData*,
                const struct timespec *);

        // Management variables
        bool writeAccess;
        bool quiet;
        bool echoOn;
        bool sync;
        std::string peer;
        std::string client;

        // Input and output buffering
        XmlParser inbuf;

        // Here are all the commands the MSR protocol supports
        void broadcast();
        void echo();
        void ping();
        void readChannel();
        void listDirectory();
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
