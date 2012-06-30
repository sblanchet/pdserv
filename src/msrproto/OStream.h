/*****************************************************************************
 *
 *  $Id$
 *
 *  copyright (C) 2012 Richard Hacker (lerichi at gmx dot net)
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

#ifndef OSTREAM_H
#define OSTREAM_H

#include <ostream>
#include <cc++/thread.h>

namespace MsrProto {

class ostream {
    public:
        ostream(std::ostream&, ost::Semaphore& sem);

        class locked {
            public:
                locked(ostream& s): os(s.os), lock(s.sem) {
                }

                operator std::ostream&() {
                    return os;
                }

            private:
                std::ostream& os;
                ost::SemaphoreLock lock;
        };

    private:
        std::ostream& os;
        ost::Semaphore& sem;
};

}
#endif //OSTREAM_H
