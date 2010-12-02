/*****************************************************************************
 * $Id$
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

namespace HRTLab {

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

        int newSignal(const Signal *);
        int newParameter(const Parameter *);

        virtual int gettime(struct timespec *) const;
        virtual int setParameters(const Parameter * const *p,
                size_t nelem, const char *data) const = 0;

        typedef std::vector<const Signal*> Signals;
        const Signals& getSignals() const;

        typedef std::vector<const Parameter*> Parameters;
        const Parameters& getParameters() const;

        const std::string name;
        const std::string version;
        const double baserate;
        const unsigned int nst;

        const Signal *getSignal(const std::string&) const;
        const Parameter *getParameter(const std::string&) const;

        // Methods used by the clients to post instructions to the real-time
        // process
        virtual Receiver* newReceiver(Session *) = 0;
        virtual unsigned int poll(const Signal * const *s, size_t nelem,
                char *buf) const = 0;

        void getSessionStatistics(std::list<SessionStatistics>&) const;

    protected:

        void parametersChanged(const Parameter * const *p,
                size_t nelem) const;

        void startProtocols();

    private:

        Task ** const task;

        MsrProto::Server *msrproto;
////    EtlProto::Server etlproto(this);

        Signals signals;
        Parameters parameters;

        typedef std::map<const std::string, const Variable*> VariableMap;
        VariableMap variableMap;
};

}
#endif // MAIN_H
