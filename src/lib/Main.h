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

//        // Methods used by the clients to post instructions to the real-time
//        // process
        int setParameter(const HRTLab::Parameter *p, const char *data) const;
        void pollParameter(const Parameter *,
                char *buf, struct timespec *) const;

        static const double bufferTime = 2;

    private:
        Task ** const task;

        mutable ost::Semaphore mutex;

        int pid;

        size_t shmem_len;
        void *shmem;

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

        char *parameterData;

        struct timespec *mtime;

        typedef std::vector<Parameter*> ParameterList;
        ParameterList parameters;

        // Methods used by the real-time process to interpret inbox
        // instructions from the clients
        bool processSdo(unsigned int tid, const struct timespec *time) const;
//
//        static int localtime(struct timespec *);

        int (* const rttime)(struct timespec*);

        // Reimplemented from HRTLab::Main
        int gettime(struct timespec *) const;
        void rxPdo(HRTLab::Session *);
        int poll(const HRTLab::Signal * const *s,
                size_t nelem, char *buf) const;
        int setParameters(const HRTLab::Parameter * const *p, size_t nelem,
                const char *data) const;
        HRTLab::Receiver *newReceiver(unsigned int tid);
};

#endif // LIB_MAIN_H
