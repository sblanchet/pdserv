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

#include <vector>
#include <list>
#include <map>
#include <string>
#include "SessionStatistics.h"

struct timespec;

namespace MsrProto {
    class Server;
}

namespace PdServ {

class Signal;
class Parameter;
class Session;
class Variable;
class Task;

class Main {
    public:
        Main(int argc, const char **argv,
                const char *name, const char *version);
        virtual ~Main();

        const std::string name;
        const std::string version;

        virtual int gettime(struct timespec *) const = 0;

        typedef std::list<const Signal*> Signals;
        const Signals& getSignals() const;
        int newSignal(Signal *);

        typedef std::list<const Parameter*> Parameters;
        const Parameters& getParameters() const;
        int newParameter(Parameter *);
        void parameterChanged(const Parameter *p, size_t startElement,
                size_t nelem) const;

        void getSessionStatistics(std::list<SessionStatistics>&) const;

        void poll( Session *session, const Signal * const *s, size_t nelem,
                char *buf) const;

    protected:
        int argc;
        const char **argv;

        Signals signals;
        Parameters parameters;

        int startProtocols();

        static int localtime(struct timespec *);

        virtual void processPoll() const = 0;

    private:
        mutable ost::Semaphore mutex;

        MsrProto::Server *msrproto;
//        EtlProto::Server etlproto(this);

        typedef std::map<const std::string, Variable*> VariableMap;
        VariableMap variableMap;
};

}
#endif // MAIN_H
