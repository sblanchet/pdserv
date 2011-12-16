/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdserv package.
 *
 *  pdserv is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pdserv is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pdserv. See COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#ifndef SUBSCRIPTION_H
#define SUBSCRIPTION_H

#include <set>

namespace MsrProto {

class PrintQ;

class Subscription {
    public:
        Subscription(const Channel *);
        ~Subscription();

        const Channel *channel;

        bool reset();

        void newValue(SubscriptionManager::PrintQ &, const void *buf);
        void print(XmlElement &parent) const;

        void set(bool event, bool sync, unsigned int decimation,
                size_t blocksize, bool base64, size_t precision);

    private:
        const size_t bufferOffset;

        bool sync;

        bool event;
        unsigned int decimation;
        unsigned int trigger;
        size_t blocksize;
        size_t nblocks;         // number of blocks to print

        size_t precision;
        bool base64;

        char *data_bptr;
        char *data_pptr;
        const char *data_eptr;
};

}
#endif //SUBSCRIPTION_H
