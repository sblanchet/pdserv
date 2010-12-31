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
        ~SubscriptionManager();

        bool unsubscribe(const Channel *s = 0);
        bool subscribe(const Channel *s,
                bool event, bool sync, unsigned int decimation,
                size_t blocksize, bool base64, size_t precision);
        bool newSignalList( const HRTLab::Signal * const *, size_t n);
        void newSignalData(MsrXml::Element *parent, const HRTLab::Receiver&);

        void sync();

    private:
        class SignalSubscription:
            public std::map<const Channel*, Subscription*> {
                public:
                    ~SignalSubscription();

                    void newSignalData(MsrXml::Element *parent, const char *);
                    void clear();
                    bool sync();
            };

        typedef std::map<const HRTLab::Signal*, SignalSubscription>
            SignalSubscriptionMap;
        SignalSubscriptionMap signalSubscriptionMap;

        typedef std::set<const HRTLab::Signal*> ActiveSignalSet;
        ActiveSignalSet activeSignalSet;
};

}
#endif //SUBSCRIPTIONMANAGER_H
