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

#ifndef LIB_RECEIVER_H
#define LIB_RECEIVER_H

#include "../Receiver.h"
#include "ShmemDataStructures.h"

#include <cc++/thread.h>
#include <map>
#include <queue>
#include <utility>      // std::pair

class Task;

class Receiver: public PdServ::Receiver {
    public:
        Receiver(Task *task, /*unsigned int tid,*/ TxFrame *start);
        ~Receiver();

        void resendSignalList();
        void newSignalList(unsigned int listId,
                const PdServ::Signal * const *, size_t n);

    private:
        Task * const task;

        unsigned int currentListId;

        ost::Semaphore mutex;
        TxFrame *rxPtr;

        mutable std::map<const PdServ::Signal*, size_t> offset;

        typedef std::pair<const unsigned int, const PdServ::Signal * const*>
            ListIdPair;

        std::queue<ListIdPair> listIdQ;

        // Reimplemented from PdServ::Receiver
        void process(PdServ::Session *);
        const char *getValue(const PdServ::Signal*) const;
};

#endif //LIB_RECEIVER_H
