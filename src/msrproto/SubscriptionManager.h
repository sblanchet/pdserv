/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdserv library.
 *
 *  The pdserv library is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or (at
 *  your option) any later version.
 *
 *  The pdserv library is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with the pdserv library. If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#ifndef SUBSCRIPTIONMANAGER_H
#define SUBSCRIPTIONMANAGER_H

#include <set>
#include <map>
#include <queue>

namespace PdServ {
    class Channel;
    class Main;
    class SessionTaskData;
}

namespace MsrProto {

class Channel;
class Subscription;
class Session;
class XmlElement;

class SubscriptionManager {
    public:
        SubscriptionManager(Session *session);
        ~SubscriptionManager();

        void clear();
        void unsubscribe(const Channel *s = 0);
        void subscribe(const Channel *s,
                bool event, bool sync, unsigned int decimation,
                size_t blocksize, bool base64, size_t precision);
        bool newSignal( const PdServ::Signal *);

        typedef std::queue<const Subscription*> PrintQ;
        void newSignalData( PrintQ &printQ,
                const PdServ::SessionTaskData *);

        void sync();

    private:
        Session * const session;

        struct SignalSubscription:
            public std::map<const Channel*, Subscription*> {
                ~SignalSubscription();

                void newSignalData( PrintQ &printQ, const void *);
                void clear();
                bool sync();
            };

        typedef std::map<const PdServ::Signal*, SignalSubscription>
            SignalSubscriptionMap;
        SignalSubscriptionMap signalSubscriptionMap;

        typedef std::set<const PdServ::Signal*> ActiveSignalSet;
        ActiveSignalSet activeSignalSet;
};

}
#endif //SUBSCRIPTIONMANAGER_H
