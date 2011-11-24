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

#ifndef MAIN_H
#define MAIN_H

#include <cc++/thread.h>

#include <list>
#include <vector>
#include <map>
#include <string>

struct timespec;

namespace MsrProto {
    class Server;
}

namespace PdServ {

class Signal;
class Parameter;
class Variable;
class Task;
class Session;
class SessionShadow;
class SessionStatistics;

class Main {
    public:
        Main(int argc, const char **argv,
                const char *name, const char *version);
        virtual ~Main();

        const std::string name;
        const std::string version;

        virtual int gettime(struct timespec *) const = 0;

        size_t numTasks() const;
        const Task *getTask(unsigned int n) const;
        const Task *getTask(double sampleTime) const;

        typedef std::list<const Signal*> Signals;
        const Signals& getSignals() const;

        typedef std::list<const Parameter*> Parameters;
        const Parameters& getParameters() const;
        void parameterChanged(const Parameter *p, size_t startElement,
                size_t nelem) const;

        void getSessionStatistics(std::list<SessionStatistics>&) const;

        void poll( Session *session, const Signal * const *s,
                size_t nelem, void *buf, struct timespec *t) const;

        const Variable *findVariable(const std::string& path) const;

        virtual SessionShadow *newSession(Session *) const = 0;

    protected:
        int argc;
        const char **argv;

        typedef std::vector<Task*> TaskList;
        TaskList task;
        Parameters parameters;
        Signals signals;

        void startServers();

        static int localtime(struct timespec *);

        virtual void processPoll(size_t delay_ms, const Signal * const *s,
                size_t nelem, void * const *pollDest,
                struct timespec *t) const = 0;

        typedef std::map<std::string, const Variable*> VariableMap;
        VariableMap variableMap;

    private:
        mutable ost::Semaphore mutex;

        MsrProto::Server *msrproto;
//        EtlProto::Server etlproto(this);

};

}
#endif // MAIN_H
