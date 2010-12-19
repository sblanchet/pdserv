/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdcomserv package.
 *
 *  pdcomserv is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pdcomserv is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pdcomserv. See COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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
