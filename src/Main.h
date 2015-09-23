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
#include <map>
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
class ProcessParameter;
class Variable;
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


        virtual std::list<const Task*> getTasks() const = 0;
        virtual std::list<const Event*> getEvents() const = 0;
        virtual std::list<const Parameter*> getParameters() const = 0;
        virtual void prepare(Session *session) const = 0;
        virtual void cleanup(const Session *session) const = 0;
        virtual const Event *getNextEvent(const Session* session,
                size_t *index, bool *state, struct timespec *t) const = 0;

        // Setting a parameter has various steps:
        // 1) client calls parameter->setValue(session, ...)
        //    This virtual method is implemented by ProcessParameter
        // 2) ProcessParameter calls
        //        main->setValue(processParameter, session, ...)
        //    so that main can check whether session is allowed to set it
        // 3) main calls processParameter->setValue(...)
        //    so that ProcessParameter can obtain write lock
        // 4) processParameter calls main->setParameter(processParameter, ...)
        //    virtual method to do the setting
        // 5) When the thread of execution returns in 3) successfully,
        //    main can do various functions, e.g.
        //      - inform clients of a value change
        //      - save persistent parameter
        //      - etc
        int setValue(const ProcessParameter* p, const Session *session,
                const char* buf, size_t offset, size_t count);
        virtual int setParameter(const ProcessParameter* p,
                size_t offset, size_t count) const = 0;

    protected:
        void setupLogging();

        static int localtime(struct timespec *);

        virtual void processPoll(
                unsigned int delay_ms, const Signal * const *s,
                size_t nelem, void * const * pollDest,
                struct timespec *t) const = 0;

        unsigned int setupPersistent();
        void savePersistent();
        virtual void initializeParameter(const Parameter* p,
                const char* data, const struct timespec* mtime,
                const Signal* s) = 0;
        virtual bool getPersistentSignalValue(const Signal *s,
                char* buf, struct timespec* time) = 0;

    private:

        mutable ost::Semaphore mutex;

        const log4cplus::Logger parameterLog;

        PdServ::Config config;

        bool persistentLogTraceOn;
        log4cplus::Logger persistentLog;
        log4cplus::Logger persistentLogTrace;
        PdServ::Config persistentConfig;
        typedef std::map<const Parameter*, const Signal*> PersistentMap;
        PersistentMap persistentMap;

        MsrProto::Server *msrproto;
        Supervisor::Server *supervisor;
//        EtlProto::Server etlproto(this);

//         int setupDaemon();

        void consoleLogging();
        void syslogLogging();
};

}
#endif // MAIN_H
