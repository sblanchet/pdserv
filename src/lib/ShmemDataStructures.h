/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef SHMEM_DATA_STRUCTURES_H
#define SHMEM_DATA_STRUCTURES_H

#include <ctime>

struct TxFrame {
    TxFrame *next;
    unsigned int signalListNo;
    unsigned int seqNo;
    struct timespec time;
    char data[];
};

#endif // SHMEM_DATA_STRUCTURES_H
