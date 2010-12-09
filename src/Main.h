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

        const std::string name;
        const std::string version;
        const double baserate;
        const unsigned int nst;

        virtual int gettime(struct timespec *) const;

        virtual void getValues( const Signal * const *, size_t nelem,
                char *buf, struct timespec * = 0) const = 0;
        virtual void getValues( const Parameter * const *, size_t nelem,
                char *buf, struct timespec * = 0) const = 0;

        typedef std::vector<const Signal*> Signals;
        const Signals& getSignals() const;
        const Signal *getSignal(const std::string&) const;
        int newSignal(Signal *);

        typedef std::vector<const Parameter*> Parameters;
        const Parameters& getParameters() const;
        const Parameter *getParameter(const std::string&) const;
        int newParameter(Parameter *);

        virtual int setParameters(const Parameter * const *p,
                size_t nelem, const char *data) const = 0;

        virtual void subscribe(const Session *,
                const Signal * const *, size_t n) const = 0;
        virtual void unsubscribe(const Session *,
                const Signal * const * = 0, size_t n = 0) const = 0;

        // Methods used by the clients to post instructions to the real-time
        // process
        virtual Receiver* newReceiver(unsigned int tid) = 0;

        void getSessionStatistics(std::list<SessionStatistics>&) const;

    protected:
        Signals signals;
        Parameters parameters;

        void parametersChanged(const Parameter * const *p,
                size_t nelem) const;

        int startProtocols();

    private:

        MsrProto::Server *msrproto;
////    EtlProto::Server etlproto(this);

        typedef std::map<const std::string, Variable*> VariableMap;
        VariableMap variableMap;
};

}
#endif // MAIN_H
