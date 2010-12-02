/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef LIB_MAIN_H
#define LIB_MAIN_H

#include "../Main.h"

#include <cc++/thread.h>
#include "rtlab/rtlab.h"

class Parameter;
class Signal;
class Variable;
class Task;

class Main: public HRTLab::Main {
    public:
        Main(int argc, const char *argv[],
                const char *name, const char *version,
                double baserate,
                unsigned int nst, const unsigned int decimation[],
                int (*gettime)(struct timespec*));
        ~Main();

        int init();
        void update(int st, const struct timespec *time) const;

        Signal *newSignal(
                const char *path,
                enum si_datatype_t datatype,
                const void *addr,
                unsigned int tid,
                unsigned int decimation,
                unsigned int ndims = 1,
                const unsigned int dim[] = 0);

        Parameter *newParameter(
                const char *path,
                enum si_datatype_t datatype,
                void *addr,
                unsigned int mode,
                unsigned int ndims = 1,
                const unsigned int dim[] = 0);

        void setVariableAlias(Variable *p, const char *alias) const;
        void setVariableUnit(Variable *p, const char *unit) const;
        void setVariableComment(Variable *p, const char *comment) const;
        void setParameterTrigger(Parameter *p,
                paramtrigger_t trigger, void *priv_data) const;

//        const std::string name;
//        const std::string version;
//        const double baserate;
//        const unsigned int nst;
//        unsigned int * const decimation;
//
//        const struct timespec *getMTime(const Parameter *) const;
//        const Signal *getSignal(const std::string&) const;
//        const Signal *getSignal(size_t) const;
//        const Parameter *getParameter(const std::string&) const;
//        const Parameter *getParameter(size_t) const;
//        const char *getParameterAddr(const Parameter *) const;
//
//        // Methods used by the clients to post instructions to the real-time
//        // process
        int setParameter(const HRTLab::Parameter *p, const char *data) const;
        void pollParameter(const Parameter *,
                char *buf, struct timespec *) const;
        void pollSignal(const Signal *,
                char *buf, struct timespec *) const;
//        unsigned int writeParameter(const Parameter * const *,
//                size_t n, const char *data, int *errorCode = 0);
//        unsigned int poll(const Signal * const *s, size_t nelem, char *buf);
//        void unsubscribe(const Session *session,
//                const Signal * const *s = 0, size_t nelem = 0);
//        void subscribe(const Session *session,
//                const Signal * const *s, size_t nelem);
//
//        void getSessionStatistics(std::list<Session::Statistics>&) const;
//
    private:
        Task ** const task;

        mutable ost::Semaphore mutex;
//
//        MsrProto::Server *msrproto;
////    EtlProto::Server etlproto(this);
//
//        int pid;
//
//        size_t shmem_len;
//        void *shmem;
//
        struct SDOStruct {
            unsigned int reqId;
            unsigned int replyId;
            enum {PollSignal, WriteParameter} type;
            int errorCode;
            unsigned int count;
            struct timespec time;
            union {
                const Signal *signal[];
                const Parameter *parameter[];
            };
        } *sdo;
        mutable ost::Semaphore sdoMutex;
        char *sdoData;
//        char *parameterData;
//        char **parameterAddr;
//
//        struct timespec *mtime;
//
//        typedef std::vector<Signal*> SignalList;
//        SignalList signals;
//
//        typedef std::vector<Parameter*> ParameterList;
//        ParameterList parameters;
//
//        typedef std::map<const std::string, const Variable*> VariableMap;
//        VariableMap variableMap;
//
//
        // Methods used by the real-time process to interpret inbox
        // instructions from the clients
        bool processSdo(unsigned int tid, const struct timespec *time) const;
//
//        static int localtime(struct timespec *);

        int (* const rttime)(struct timespec*);

        // Reimplemented from HRTLab::Main
        int gettime(struct timespec *) const;
        void rxPdo(HRTLab::Session *);
        unsigned int poll(const HRTLab::Signal * const *s,
                size_t nelem, char *buf) const;
        int setParameters(const HRTLab::Parameter * const *p, size_t nelem,
                const char *data) const;
        HRTLab::Receiver *newReceiver(HRTLab::Session *);
};

#endif // LIB_MAIN_H
