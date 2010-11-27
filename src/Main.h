/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef MAIN_H
#define MAIN_H

#include <vector>
#include <map>
#include <list>
#include <algorithm>
#include <string>
#include <stdint.h>

#include <rtlab/rtlab.h>
#include <cc++/thread.h>

#include "Session.h"

namespace MsrProto {
    class Server;
}

namespace HRTLab {

class Signal;
class Parameter;
class Variable;
class Task;

class Main {
    public:
        Main(int argc, const char *argv[],
                const char *name, const char *version,
                double baserate,
                unsigned int nst, const unsigned int decimation[],
                int (*gettime)(struct timespec*));
        ~Main();

        void update(int st, const struct timespec *time);
        int start();

        Signal *newSignal(
                const char *path,
                enum si_datatype_t datatype,
                const void *addr,
                unsigned int tid,
                unsigned int decimation,
                unsigned int ndims = 1,
                const unsigned int dim[] = 0
                );

        Parameter *newParameter(
                const char *path,
                enum si_datatype_t datatype,
                void *addr,
                unsigned int mode,
                unsigned int ndims = 1,
                const unsigned int dim[] = 0
                );


        const std::string name;
        const std::string version;
        const double baserate;
        const unsigned int nst;
        unsigned int * const decimation;
        int (* const gettime)(struct timespec*);

        const struct timespec *getMTime(const Parameter *) const;
        const Signal *getSignal(const std::string&) const;
        const Signal *getSignal(size_t) const;
        const Parameter *getParameter(const std::string&) const;
        const Parameter *getParameter(size_t) const;
        const char *getParameterAddr(const Parameter *) const;

        // Methods used by the clients to post instructions to the real-time
        // process
        void newSession(const Session *);
        void rxPdo(Session *);
        void closeSession(const Session *s);
        unsigned int writeParameter(const Parameter * const *,
                size_t n, const char *data, int *errorCode = 0);
        unsigned int poll(const Signal * const *s, size_t nelem, char *buf);
        void unsubscribe(const Session *session,
                const Signal * const *s = 0, size_t nelem = 0);
        void subscribe(const Session *session,
                const Signal * const *s, size_t nelem);

        void getSessionStatistics(std::list<Session::Statistics>&) const;

    private:

        mutable ost::Semaphore mutex;

        MsrProto::Server *msrproto;
//    EtlProto::Server etlproto(this);

        int pid;

        size_t shmem_len;
        void *shmem;

        struct SDOStruct {
            unsigned int reqId;
            unsigned int replyId;
            enum {PollSignal, WriteParameter} type;
            unsigned int errorCount;
            unsigned int count;
            struct timespec time;
            union {
                const Signal *signal;
                const Parameter *parameter;
                int errorCode;
            } data[];
        } *sdo;
        mutable ost::Semaphore sdoMutex;
        char *sdoData;
        char *parameterData;
        char **parameterAddr;

        struct timespec *mtime;

        typedef std::vector<Signal*> SignalList;
        SignalList signals;

        typedef std::vector<Parameter*> ParameterList;
        ParameterList parameters;

        typedef std::map<const std::string, const Variable*> VariableMap;
        VariableMap variableMap;

        Task **task;

        // Methods used by the real-time process to interpret inbox
        // instructions from the clients
        void processSdo(unsigned int tid, const struct timespec *time);

        static int localtime(struct timespec *);
};

}
#endif // MAIN_H
