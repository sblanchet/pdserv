/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef LIB_RECEIVER_H
#define LIB_RECEIVER_H

#include "../Receiver.h"
#include "ShmemDataStructures.h"

#include <cc++/thread.h>
#include <map>
#include <queue>
#include <utility>      // std::pair

class Task;

class Receiver: public HRTLab::Receiver {
    public:
        Receiver(Task *task, unsigned int tid, TxFrame *start);
        ~Receiver();

        void newSignalList(unsigned int listId,
                const HRTLab::Signal * const *, size_t n);

    private:
        Task * const task;

        unsigned int currentListId;

        ost::Semaphore mutex;
        TxFrame *rxPtr;

        mutable std::map<const HRTLab::Signal*, size_t> offset;

        typedef std::pair<const unsigned int, const HRTLab::Signal * const*>
            ListIdPair;

        std::queue<ListIdPair> listIdQ;

        // Reimplemented from HRTLab::Receiver
        void process(HRTLab::Session *);
        const char *getValue(const HRTLab::Signal*) const;
};

#endif //LIB_RECEIVER_H
