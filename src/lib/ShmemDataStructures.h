/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef SHMEM_DATA_STRUCTURES_H
#define SHMEM_DATA_STRUCTURES_H

#include <ctime>

class Signal;

struct Instruction {
    enum Type {Clear, Insert, Remove} instruction;
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
            const Signal *signal[];
        } list;
    };
};

#endif // SHMEM_DATA_STRUCTURES_H
