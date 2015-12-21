/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdserv library.
 *
 *  The pdserv library is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or (at
 *  your option) any later version.
 *
 *  The pdserv library is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with the pdserv library. If not, see <http://www.gnu.org/licenses/>.
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

#define MSR_FEATURES "pushparameters,binparameters,eventchannels,statistics,pmtime,aic,messages,polite,list"

/* pushparameters: Parameter werden vom Echtzeitprozess an den Userprozess gesendet bei Änderung
   binparameters: Parameter können Binär übertragen werden
   eventchannels: Kanäle werden nur bei Änderung übertragen
   statistics: Statistische Informationen über die anderen verbundenen Echtzeitprozesses
   pmtime: Die Zeit der Änderungen eines Parameters wird vermerkt und übertragen
   aic: ansychrone Input Kanäle
   messages: Kanäle mit dem Attribut: "messagetyp" werden von der msr_lib überwacht und bei Änderung des Wertes als Klartextmeldung verschickt V:6.0.10
   polite: Server will not send any messages such as <pu> or <log> by itself
   list: Server understands <list> command
*/

#include "../Session.h"
#include "XmlParser.h"
#include "XmlElement.h"

#include <cc++/thread.h>
#include <cc++/socketport.h>
#include <vector>
#include <set>
#include <cstdio>

struct timespec;

namespace PdServ {
    class Parameter;
    class SessionTaskData;
    class Task;
    class TaskStatistics;
    class SessionStatistics;
}

namespace MsrProto {

class SubscriptionManager;
class TaskStatistics;
class SessionStatistics;
class Server;
class Parameter;
class Subscription;

class Session: public ost::Thread, public PdServ::Session {
    public:
        Session( Server *s, ost::TCPSocket *socket);
        ~Session();

        void broadcast(Session *s, const struct timespec& ts,
                const std::string &action, const std::string &element);
        void parameterChanged(const Parameter*);
        void setAIC(const Parameter* p);
        void getSessionStatistics(PdServ::SessionStatistics &stats) const;

        const struct timespec *getTaskTime(size_t taskId) const;
        const PdServ::TaskStatistics *getTaskStatistics(size_t taskId) const;

        double *getDouble() const {
            return &tmp.dbl;
        }

        class TCPStream: public ost::Socket, public std::streambuf {
            public:
                TCPStream(ost::TCPSocket *socket);
                ~TCPStream();

                int read(char *buf, size_t n, timeout_t timeout);

                XmlElement createElement(const char *name);

                size_t inBytes;
                size_t outBytes;

                std::string peer;
                std::string commandId;

            private:
                std::ostream os;

                FILE *file;

                // Reimplemented from std::streambuf
                int overflow ( int c );
                std::streamsize xsputn ( const char * s, std::streamsize n );
                int sync();
        };

    private:
        Server * const server;

        TCPStream tcp;

        std::ostream xmlstream;

        // Protection for inter-session communication
        ost::Semaphore mutex;

        // List of parameters that have changed
        typedef std::set<const Parameter*> ParameterSet;
        ParameterSet changedParameter;

        // Asynchronous input channels.
        // These are actually parameters that are misused as input streams.
        // Parameters in this set are not announced as changed as often as
        // real parameters are.
        typedef std::set<const PdServ::Parameter*> MainParameterSet;
        MainParameterSet aic;
        size_t aicDelay;        // When 0, notify that aic's have changed

        // Broadcast list.
        typedef struct {
            struct timespec ts;
            std::string action;
            std::string message;
        } Broadcast;
        typedef std::list<const Broadcast*> BroadcastList;
        BroadcastList broadcastList;

        typedef std::vector<SubscriptionManager*> SubscriptionManagerVector;
        SubscriptionManagerVector subscriptionManager;
        const SubscriptionManager *timeTask;

        // Temporary memory space needed to handle statistic channels
        union {
            uint32_t uint32;
            double dbl;
        } mutable tmp;

        // Reimplemented from ost::Thread
        void initial();
        void run();
        void final();

        void processCommand(const XmlParser::Element&);
        // Management variables
        bool writeAccess;
        bool quiet;
        bool polite;
        bool echoOn;
        std::string peer;
        std::string client;

        // Here are all the commands the MSR protocol supports
        void broadcast(const XmlParser::Element&);
        void echo(const XmlParser::Element&);
        void ping(const XmlParser::Element&);
        void readChannel(const XmlParser::Element&);
        void listDirectory(const XmlParser::Element&);
        void readParameter(const XmlParser::Element&);
        void readParamValues(const XmlParser::Element&);
        void readStatistics(const XmlParser::Element&);
        void remoteHost(const XmlParser::Element&);
        void writeParameter(const XmlParser::Element&);
        void xsad(const XmlParser::Element&);
        void xsod(const XmlParser::Element&);
};

}
#endif //MSRSESSION_H
