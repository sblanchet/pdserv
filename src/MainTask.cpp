/***************************************************************************
 * $Id$
 **************************************************************************/

#include "Main.h"
#include "Signal.h"
#include <unistd.h>

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

using namespace HRTLab;

const size_t Main::Task::dtype_idx[9] = {0,0,0,0,0,1,0,2,3};

/////////////////////////////////////////////////////////////////////////////
Main::Task::Task(Main *main, unsigned int tid):
    main(main), tid(tid)
{
    std::fill_n(subscriptionIndex, 4, 0);
    dirty = false;

    size_t signalCount= 0;
    for (size_t i = 0; i < main->signals.size(); i++) {
        signalCount += main->signals[i]->tid == tid ? 1 : 0;
    }

    copyList = new CopyList[signalCount + 1];
    copyListEnd = copyList;
    copyList->begin = 0;

    blockLengthBytes = sizeof(struct timespec);
    blockLength = element_count_uint(blockLengthBytes) + 4;
}

/////////////////////////////////////////////////////////////////////////////
Main::Task::~Task()
{
    delete[] copyList;
}

/////////////////////////////////////////////////////////////////////////////
void Main::Task::subscribe(unsigned int index)
{
    dirty = true;

    if (main->subscriptionPtr[index])
        return;

    const Signal *signal = main->signals[index];

    const size_t dtype = dtype_idx[signal->width];
    CopyList *p = copyList;
    for (size_t i = 0; i <= dtype; i++) {
        p += subscriptionIndex[i];
    }
    subscriptionIndex[dtype]++;
    main->subscriptionPtr[index] = p;

    std::copy(p, copyListEnd++, p + 1);
    p->begin = signal->addr;
    p->end = signal->addr + signal->memSize;
    p->len = signal->memSize;
    p->sigIdx = index;

    blockLengthBytes += signal->memSize;
    blockLength = element_count_uint(blockLengthBytes) + 4;

    cout << __LINE__
        << ' ' << index
        << ' ' << dtype
        << ' ' << p - copyList
        << ' ' << (void*)p->begin
        << ' ' << (void*)p->end
        << ' ' << p->len
        << ' ' << p->sigIdx << endl;
}

/////////////////////////////////////////////////////////////////////////////
void Main::Task::unsubscribe(unsigned int index)
{
    if (main->subscriptionPtr[index] == 0)
        return;

    dirty = true;

    const Signal *signal = main->signals[index];

    subscriptionIndex[dtype_idx[signal->width]]--;
    CopyList *p = main->subscriptionPtr[index];

    std::copy(p + 1, copyListEnd, p);
    (--copyListEnd)->begin = 0;

    main->subscriptionPtr[index] = 0;
    blockLengthBytes -= signal->memSize;
    blockLength = element_count_uint(blockLengthBytes) + 4;
}

/////////////////////////////////////////////////////////////////////////////
void Main::Task::update(const struct timespec *time)
{
    unsigned int *buf = main->signal_ptr;

    if (dirty) {
        dirty = false;

        // Copy new signal lists to outbox
        // If the signal list changed, write the following:
        // [0]: SubscriptionList
        // [1]: Size of this block
        // [2]: Task id
        // [3]: Decimation of the block
        // [4..]: Signal index of subscribed signals

        size_t blockLen = copyListEnd - copyList + 4;
        if (buf + blockLen >= main->signal_ptr_end) {
            buf = main->signal_ptr_start;
            *buf = 0;
            *main->signal_ptr = Restart;
            main->signal_ptr = buf;
        }
        *++buf = blockLen;
        *++buf = tid;   
        *++buf = 1;               // Transmission decimation

        for (CopyList *p = copyList; p->begin; p++)
            *++buf = p->sigIdx;

        *++buf = 0;
        *main->signal_ptr = SubscriptionList;
        main->signal_ptr = buf;
    }

    if (buf + blockLength >= main->signal_ptr_end) {
        buf = main->signal_ptr_start;
        *buf = 0;
        *main->signal_ptr = Restart;
        main->signal_ptr = buf;
    }

    buf[1] = blockLength;
    buf[2] = tid;
    buf[3] = iterationNo++;

    char *dataPtr = reinterpret_cast<char*>(buf + 4);

    if (time)
        *reinterpret_cast<struct timespec*>(dataPtr) = *time;
    else
        std::fill_n(dataPtr, sizeof(struct timespec), 0);
    dataPtr += sizeof(struct timespec);                 // TimeSpec

    const CopyList *c = copyList;
    while (c->begin) {
        std::copy(c->begin, c->end, dataPtr);
        dataPtr += c->len;
        c++;
    }

    buf[blockLength] = 0;
    buf[0] = SubscriptionData;

    main->signal_ptr += blockLength;
}
