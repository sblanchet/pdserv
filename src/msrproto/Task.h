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

#ifndef MSRTASK_H
#define MSRTASK_H

#include <set>

namespace HRTLab {
    class Main;
    class Receiver;
    class Signal;
    class Parameter;
}

namespace MsrXml {
    class Element;
}

namespace MsrProto {

class Session;
class SubscriptionChange;

class Task {
    public:
        Task(SubscriptionChange&);
        ~Task();

        void delSignal(const HRTLab::Signal *s = 0);
        void addSignal(const HRTLab::Signal *s, unsigned int idx,
                bool event, bool sync, unsigned int decimation,
                size_t blocksize, bool base64, size_t precision);

        bool newSignalList(const HRTLab::Signal * const *s, size_t n);
        void newSignalValues(MsrXml::Element *, const HRTLab::Receiver&);


        void sync();

    private:

        SubscriptionChange& subscriptionChange;

        struct SignalData {
            const HRTLab::Signal *signal;
            MsrXml::Element *element;
            bool event;
            bool sync;
            unsigned int decimation;
            unsigned int trigger;
            size_t blocksize;
            size_t sigMemSize;
            void (MsrXml::Element::*printValue)(const char *,
                    const HRTLab::Variable *v, const char* data,
                    size_t precision, size_t n);
            size_t precision;
            char *data_bptr;
            char *data_pptr;
            char *data_eptr;
        };

        typedef std::map<const HRTLab::Signal *, SignalData*> ActiveSet;
        ActiveSet activeSet;

        typedef std::map<const HRTLab::Signal*, SignalData> SubscribedSet;
        SubscribedSet subscribedSet;
};

}
#endif //MSRTASK_H
