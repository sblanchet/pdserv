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

#ifndef SUBSCRIPTION_H
#define SUBSCRIPTION_H

#include <set>

namespace HRTLab {
//    class Main;
//    class Receiver;
//    class Signal;
//    class Parameter;
}

namespace MsrXml {
    class Element;
}

namespace MsrProto {

class Subscription {
    public:
        Subscription(const Channel *, SubscriptionManager&);
        ~Subscription();

        const Channel *channel;

        bool activate();
        void sync();
        void newValue(MsrXml::Element *parent,
                const HRTLab::Receiver &receiver);

        void set(bool event, bool sync, unsigned int decimation,
                size_t blocksize, bool base64, size_t precision);

    private:
        SubscriptionManager& subscriptionManager;
        MsrXml::Element element;

        bool _sync;
        bool active;

        bool event;
        unsigned int decimation;
        unsigned int trigger;
        size_t blocksize;

        size_t precision;
        void (MsrXml::Element::*printValue)( const char *attribute,
                const char* data, const HRTLab::Variable *v, size_t count,
                size_t precision);

        char *data_bptr;
        char *data_pptr;
        char *data_eptr;
};

}
#endif //SUBSCRIPTION_H
