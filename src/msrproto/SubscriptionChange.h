/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef SUBSCRIPTIONCHANGE_H
#define SUBSCRIPTIONCHANGE_H

#include <set>

namespace HRTLab {
    class Signal;
    class Main;
    class Session;
}

namespace MsrProto {

class SubscriptionChange {
    public:
        SubscriptionChange(HRTLab::Main *main,
                const HRTLab::Session *);

        void subscribe(const HRTLab::Signal *s);
        void unsubscribe(const HRTLab::Signal *s);

        void process();

    private:
        HRTLab::Main * const main;
        const HRTLab::Session * const session;

        unsigned int state;

        typedef std::set<const HRTLab::Signal*> SignalSet;
        SignalSet unsubscribeSet;
        SignalSet subscribeSet;
};

}
#endif //SUBSCRIPTIONCHANGE_H
