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

#ifndef SUBSCRIPTION_H
#define SUBSCRIPTION_H

namespace MsrProto {

class Subscription {
    public:
        Subscription(const Channel *, bool event, unsigned int decimation,
                size_t blocksize, bool base64, size_t precision);
        ~Subscription();

        const Channel *channel;
        const bool event;
        const unsigned int decimation;

        bool newValue(const char *buf);
        void print(XmlElement &parent);

    private:
        const size_t bufferOffset;

        unsigned int trigger;
        unsigned int trigger_start;
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
