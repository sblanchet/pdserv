/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef RECEIVER_H
#define RECEIVER_H

#include <list>
#include <map>

struct timespec;

namespace HRTLab {

class Session;
class Signal;

class Receiver {
    public:
        Receiver(unsigned int tid);
        virtual ~Receiver();

        unsigned int const tid;
        unsigned int seqNo;
        const struct timespec *time;

        virtual const char *getValue(const Signal*) const = 0;
        virtual void process(Session *) = 0;

    private:
};

}
#endif //RECEIVER_H
