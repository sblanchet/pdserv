/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef LIB_RECEIVER_H
#define LIB_RECEIVER_H

#include "../Receiver.h"
#include "ShmemDataStructures.h"

namespace HRTLab {
    class Session;
}

class Receiver: public HRTLab::Receiver {
    public:
        Receiver();
        ~Receiver();

    private:
        mutable TxFrame *rxPtr;

        // Reimplemented from HRTLab::Receiver
        const Receiver *receive(HRTLab::Session *) const;
};

#endif //LIB_RECEIVER_H
