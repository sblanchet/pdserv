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

#include <cc++/thread.h>

#include <list>
#include <vector>
#include <string>

struct timespec;

namespace log4cpp {
    class Category;
}

namespace MsrProto {
    class Server;
}

namespace PdServ {

class Signal;
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

        const std::string name;
        const std::string version;

        virtual int gettime(struct timespec *) const = 0;

        size_t numTasks() const;
        const Task *getTask(unsigned int n) const;
        const Task *getTask(double sampleTime) const;

        typedef std::list<const ProcessParameter*> ProcessParameters;
        const ProcessParameters& getParameters() const;
        int setParameter(const Session *, const ProcessParameter *p,
                size_t startIndex, size_t nelem, const char *data) const;

        void getSessionStatistics(std::list<SessionStatistics>&) const;

        void poll( Session *session, const Signal * const *s,
                size_t nelem, void *buf, struct timespec *t) const;

    protected:

        typedef std::vector<Task*> TaskList;
        TaskList task;

        ProcessParameters parameters;

        void startServers(const Config &);

        static int localtime(struct timespec *);

        virtual void processPoll(
                unsigned int delay_ms, const Signal * const *s,
                size_t nelem, void * const * pollDest,
                struct timespec *t) const = 0;

    private:
        mutable ost::Semaphore mutex;

        log4cpp::Category &parameterLog;

        MsrProto::Server *msrproto;
//        EtlProto::Server etlproto(this);

};

}
#endif // MAIN_H
