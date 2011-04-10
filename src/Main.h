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
class Receiver;

class Main {
    public:
        Main( const char *name, const char *version,
                double baserate, unsigned int nst);
        virtual ~Main();

        const std::string name;
        const std::string version;
        const double baserate;
        const unsigned int nst;

        virtual int gettime(struct timespec *) const;

        virtual void getValues( const Signal * const *, size_t nelem,
                char *buf, struct timespec * = 0) const = 0;

        typedef std::list<const Signal*> Signals;
        const Signals& getSignals() const;
        int newSignal(Signal *);

        typedef std::list<const Parameter*> Parameters;
        const Parameters& getParameters() const;
        int newParameter(Parameter *);
        void parameterChanged(const Parameter *p, size_t startElement,
                size_t nelem) const;

        virtual void subscribe(Session *,
                const Signal * const *, size_t n) const = 0;
        virtual void unsubscribe(Session *,
                const Signal * const * = 0, size_t n = 0) const = 0;

        // Methods used by the clients to post instructions to the real-time
        // process
        virtual Receiver* newReceiver(unsigned int tid) = 0;

        void getSessionStatistics(std::list<SessionStatistics>&) const;

    protected:
        Signals signals;
        Parameters parameters;
        bool traditionalMSR;


        int startProtocols();

    private:
        MsrProto::Server *msrproto;
////    EtlProto::Server etlproto(this);

        typedef std::map<const std::string, Variable*> VariableMap;
        VariableMap variableMap;
};

}
#endif // MAIN_H
