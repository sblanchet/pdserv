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

#ifndef MAIN_H
#define MAIN_H

#include <list>
#include <vector>
#include <string>
#include <cc++/thread.h>
#include <log4cplus/logger.h>

#include "Config.h"

struct timespec;

namespace MsrProto {
    class Server;
}

namespace Supervisor {
    class Server;
}

namespace PdServ {

class Signal;
class Event;
class Parameter;
class Task;
class Session;
class Config;
class SessionStatistics;

class Main {
    public:
        Main(const std::string& name, const std::string& version);
        virtual ~Main();

        const std::string name;         /**< Application name */
        const std::string version;      /**< Application version */

        void startServers();
        void stopServers();
        void setConfig(const Config& c);

        // Get the current system time.
        // Reimplement this method in the class specialization
        virtual int gettime(struct timespec *) const;

        void getSessionStatistics(std::list<SessionStatistics>&) const;

        // Poll the current value of a list of signals
        void poll(const Session *session, const Signal * const *s,
                size_t nelem, void *buf, struct timespec *t) const;

        int setParameter(const Parameter* p, const Session *session,
                size_t offset, size_t count) const;

        virtual std::list<const Task*> getTasks() const = 0;
        virtual std::list<const Event*> getEvents() const = 0;
        virtual std::list<const Parameter*> getParameters() const = 0;
        virtual void prepare(Session *session) const = 0;
        virtual void cleanup(const Session *session) const = 0;
        virtual const Event *getNextEvent(const Session* session,
                size_t *index, bool *state, struct timespec *t) const = 0;
        virtual int setParameter(const Parameter* p,
                size_t offset, size_t count) const = 0;

    protected:
        void setupLogging();

        static int localtime(struct timespec *);

        virtual void processPoll(
                unsigned int delay_ms, const Signal * const *s,
                size_t nelem, void * const * pollDest,
                struct timespec *t) const = 0;

    private:

        mutable ost::Semaphore mutex;
        mutable ost::Semaphore parameterMutex;

        const log4cplus::Logger parameterLog;

        PdServ::Config config;

        MsrProto::Server *msrproto;
        Supervisor::Server *supervisor;
//        EtlProto::Server etlproto(this);

//         int setupDaemon();

        void consoleLogging();
        void syslogLogging();
};

}
#endif // MAIN_H
