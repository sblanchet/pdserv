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

class Main {
    public:
        Main( const char *name, const char *version,
                double baserate, unsigned int nst);
        ~Main();

        virtual int gettime(struct timespec *) const;
        virtual int setParameters(const Parameter * const *p,
                size_t nelem, const char *data) const = 0;

        void getSignals( std::vector<const Signal*>&) const;
        void getParameters( std::vector<const Parameter*>&) const;

        std::vector<const Signal*> getSignals() const;
        std::vector<const Parameter*> getParameters() const;

        virtual void update(int st, const struct timespec *time) const = 0;
//        int start();
//
//        Signal *newSignal(
//                const char *path,
//                enum si_datatype_t datatype,
//                const void *addr,
//                unsigned int tid,
//                unsigned int decimation,
//                unsigned int ndims = 1,
//                const unsigned int dim[] = 0
//                );
//
//        Parameter *newParameter(
//                const char *path,
//                enum si_datatype_t datatype,
//                void *addr,
//                unsigned int mode,
//                unsigned int ndims = 1,
//                const unsigned int dim[] = 0
//                );
//
//
        const std::string name;
        const std::string version;
        const double baserate;
        const unsigned int nst;
//        int (* const gettime)(struct timespec*);
//
        const Signal *getSignal(const std::string&) const;
        const Parameter *getParameter(const std::string&) const;
//        const char *getParameterAddr(const Parameter *) const;
//
        // Methods used by the clients to post instructions to the real-time
        // process
        void newSession(const Session *);
        void closeSession(const Session *s);

        virtual void rxPdo(Session *) = 0;
//        unsigned int writeParameter(const Parameter * const *,
//                size_t n, const char *data, int *errorCode = 0);
        virtual unsigned int poll(const Signal * const *s, size_t nelem,
                char *buf) const = 0;
//        void unsubscribe(const Session *session,
//                const Signal * const *s = 0, size_t nelem = 0);
//        void subscribe(const Session *session,
//                const Signal * const *s, size_t nelem);

        void getSessionStatistics(std::list<SessionStatistics>&) const;

    protected:
        int newSignal(const Signal *);
        int newParameter(const Parameter *);

        void parametersChanged(const Parameter * const *p,
                size_t nelem) const;

    private:

        Task ** const task;

//        mutable ost::Semaphore mutex;
//
        MsrProto::Server *msrproto;

        typedef std::vector<const Signal*> SignalList;
        SignalList signals;

        typedef std::vector<const Parameter*> ParameterList;
        ParameterList parameters;

        typedef std::map<const std::string, const Variable*> VariableMap;
        VariableMap variableMap;

////    EtlProto::Server etlproto(this);
//
//        int pid;
//
//        size_t shmem_len;
//        void *shmem;
//
//        struct SDOStruct {
//            unsigned int reqId;
//            unsigned int replyId;
//            enum {PollSignal, WriteParameter} type;
//            unsigned int errorCount;
//            unsigned int count;
//            struct timespec time;
//            union {
//                const Signal *signal;
//                const Parameter *parameter;
//                int errorCode;
//            } data[];
//        } *sdo;
//        mutable ost::Semaphore sdoMutex;
//        char *sdoData;
//        char *parameterData;
//        char **parameterAddr;
//
//        struct timespec *mtime;
//
//
//
//        // Methods used by the real-time process to interpret inbox
//        // instructions from the clients
//        void processSdo(unsigned int tid, const struct timespec *time);
//
};

}
#endif // MAIN_H
