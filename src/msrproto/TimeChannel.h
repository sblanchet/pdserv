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

#ifndef MSRTIMECHANNEL_H
#define MSRTIMECHANNEL_H

#include "../Signal.h"

namespace MsrProto {

class DirectoryNode;
class Server;

class TimeChannel: public PdServ::Signal {
    public:
        TimeChannel( Server *s);
        ~TimeChannel();

        const Server * const server;

    private:

        // Reimplemented from PdServ::Signal
        void subscribe(PdServ::Session *) const {
        }
        void unsubscribe(PdServ::Session *) const {
        }

        double poll(const PdServ::Session *s, char *buf) const;

        // Reimplemented from PdServ::Variable
        void getValue(PdServ::Session*,
                char *, struct timespec * = 0) const;
};

}

#endif //MSRTIMECHANNEL_H
