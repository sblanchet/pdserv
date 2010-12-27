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

#ifndef SUBSCRIPTIONMANAGER_H
#define SUBSCRIPTIONMANAGER_H

#include <set>
#include <map>

namespace HRTLab {
    class Channel;
    class Main;
}

namespace MsrXml {
    class Element;
}

namespace MsrProto {

class Channel;
class Subscription;
class Session;

class SubscriptionManager {
    public:
        SubscriptionManager(Session *);
        ~SubscriptionManager();

        void unsubscribe(const Channel *s = 0);
        void subscribe(const Channel *s,
                bool event, bool sync, unsigned int decimation,
                size_t blocksize, bool base64, size_t precision);
        void newSignalList(const HRTLab::Task *task,
                const HRTLab::Signal * const *, size_t n);
        void newSignalData(MsrXml::Element *parent,
                const HRTLab::Receiver&, const char *data);

        void sync();

        bool active(const HRTLab::Signal *s) const;

        typedef std::set<const Channel*> ChannelSet;
        const ChannelSet& getActiveChannels(const HRTLab::Signal *s) const;

    private:
        Session * const session;

        typedef std::map<const Channel *, Subscription*>
            ChannelSubscriptionMap;
        typedef std::map<const HRTLab::Signal*, ChannelSubscriptionMap>
            SignalSubscriptionMap;
        typedef std::map<const HRTLab::Task*, SignalSubscriptionMap>
            TaskSubscription;
        TaskSubscription taskSubscription;
};

}
#endif //SUBSCRIPTIONMANAGER_H
