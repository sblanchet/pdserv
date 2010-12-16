/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef LIB_MAIN_H
#define LIB_MAIN_H

#include "../Main.h"

#include <cc++/thread.h>
#include <set>
#include "rtlab/rtlab.h"

class Receiver;
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

        Task ** const task;

        int init();
        void update(int st, const struct timespec *time) const;

//        void newSignalList(unsigned int listId,
//                const HRTLab::Signal * const *, size_t n) const;

        static const double bufferTime = 2;

    private:
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
        struct timespec *sdoTaskTime;
        char *sdoData;

        // Methods used by the real-time process to interpret inbox
        // instructions from the clients
        bool processSdo(unsigned int tid, const struct timespec *time) const;
//
//        static int localtime(struct timespec *);

        int (* const rttime)(struct timespec*);

        // Reimplemented from HRTLab::Main
        int gettime(struct timespec *) const;
        void rxPdo(HRTLab::Session *);
        void getValues(const HRTLab::Signal * const *s,
                size_t nelem, char *buf, struct timespec * = 0) const;
        void getValues(const HRTLab::Parameter * const *p,
                size_t nelem, char *buf, struct timespec * = 0) const;
        int setParameters(const HRTLab::Parameter * const *p, size_t nelem,
                const char *data) const;
        HRTLab::Receiver *newReceiver(unsigned int tid);
        void subscribe(const HRTLab::Session *,
                const HRTLab::Signal * const *, size_t n) const;
        void unsubscribe(const HRTLab::Session *,
                const HRTLab::Signal * const * = 0, size_t n = 0) const;
};

#endif // LIB_MAIN_H
