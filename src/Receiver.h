/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef RECEIVER_H
#define RECEIVER_H

#include <list>

struct timespec;

namespace HRTLab {

class Session;
class Signal;

class Receiver {
    public:
        Receiver();
        virtual ~Receiver();

        unsigned int tid;
        struct timespec *time;
        enum {NewSignals, Data, NOOP} type;

        typedef std::list<const Signal*> SignalList;
        const SignalList& getSignals() const;
        
        virtual const char *getValue(const Signal*) const = 0;

    private:
};

}
#endif //RECEIVER_H
