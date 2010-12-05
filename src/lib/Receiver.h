/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef LIB_RECEIVER_H
#define LIB_RECEIVER_H

#include "../Receiver.h"
#include "ShmemDataStructures.h"

class Receiver: public HRTLab::Receiver {
    public:
        Receiver(unsigned int tid);
        ~Receiver();

    private:
        mutable TxFrame *rxPtr;

        std::map<const HRTLab::Signal *, size_t> offset;
        const char *valueBuf;

        // Reimplemented from HRTLab::Receiver
        void process(HRTLab::Session *);
        const char *getValue(const HRTLab::Signal*) const;
};

#endif //LIB_RECEIVER_H
