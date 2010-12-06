/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef SHMEM_DATA_STRUCTURES_H
#define SHMEM_DATA_STRUCTURES_H

#include <ctime>

namespace HRTLab {
    class Signal;
}

class Signal;

struct Instruction {
    enum Type {Clear = 0, Insert, Remove} instruction;
    const Signal *signal;
};

struct TxFrame {
    TxFrame *next;
    enum {PdoData = 1, PdoList} type;
    union {
        struct {
            unsigned int seqNo;
            struct timespec time;
            char data[];
        } pdo;
        struct {
            unsigned int count;
            const HRTLab::Signal *signal[];
        } list;
    };
};

#endif // SHMEM_DATA_STRUCTURES_H
