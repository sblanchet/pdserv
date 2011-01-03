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

#ifndef SESSION_H
#define SESSION_H

#include <string>
#include <ctime>

#include "SessionStatistics.h"

namespace HRTLab {

class Task;
class Signal;
class Main;
class Receiver;

class Session {
    public:
        Session(Main *main);
        ~Session();

        SessionStatistics getStatistics() const;

        // These methods are called from within the context of rxPdo
        virtual void newSignalList(const Task *,
                const Signal * const *, size_t n) = 0;
        virtual void newSignalData(const Receiver&) = 0;

        void rxPdo();
        void resendSignalList(const Task *) const;

    protected:
        Main * const main;

        size_t inBytes;
        size_t outBytes;
        std::string remoteHost;
        std::string client;

    private:
        Receiver ** const receiver;

        struct timespec connectedTime;
};

}
#endif //SESSION_H
