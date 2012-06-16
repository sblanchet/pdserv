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

#include "../SessionTask.h"

#include <ostream>
#include <set>
#include <map>
#include <queue>

namespace PdServ {
    class Channel;
    class Main;
    class Task;
    class TaskStatistics;
}

namespace MsrProto {

class Channel;
class Subscription;
class Session;
class XmlElement;

class SubscriptionManager: public PdServ::SessionTask {
    public:
        SubscriptionManager(const Session *session, const PdServ::Task*);

        const Session * const session;

        void rxPdo(std::ostream& os, ost::Semaphore& streamlock, bool quiet);

        void clear();
        void unsubscribe(const Channel *s);
        void subscribe(const Channel *s,
                bool event, unsigned int decimation,
                size_t blocksize, bool base64, size_t precision);

        void sync();

        const struct timespec *taskTime;
        const PdServ::TaskStatistics *taskStatistics;

    private:

        class SignalSubscription:
            public std::map<const Channel*, Subscription*> {
                public:
                ~SignalSubscription();

                void clear();
                void sync();
            };

        typedef std::map<const PdServ::Signal*, SignalSubscription>
            SignalSubscriptionMap;
        SignalSubscriptionMap signalSubscriptionMap;

        typedef std::set<SignalSubscription*> ActiveSignalSet;
        ActiveSignalSet activeSignalSet;

        // Reimplemented from PdServ::SessionTask
        void newSignal( const PdServ::Signal *);
};

}
#endif //SUBSCRIPTIONMANAGER_H
