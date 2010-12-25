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
    class Session;
}

namespace MsrProto {

class Channel;

class SubscriptionManager {
    public:
        SubscriptionManager(HRTLab::Main *main,
                const HRTLab::Session *);

        void subscribe(const Channel *s);
        void unsubscribe(const Channel *s);

        void process();

        bool active(const HRTLab::Signal *s) const;

        typedef std::set<const Channel*> ChannelSet;
        const ChannelSet& getActiveChannels(const HRTLab::Signal *s) const;

    private:
        HRTLab::Main * const main;
        const HRTLab::Session * const session;

        unsigned int state;

        typedef std::set<const HRTLab::Signal*> MainSignalSet;
        MainSignalSet pendingUnsubscriptions;
        MainSignalSet pendingSubscriptions;

        typedef std::map<const HRTLab::Signal*, ChannelSet> ChannelMap;
        ChannelMap channelMap;
};

}
#endif //SUBSCRIPTIONMANAGER_H
