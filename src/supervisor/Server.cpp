/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2012 Richard Hacker (lerichi at gmx dot net)
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

#include "Server.h"
#include "../Main.h"
#include "../Event.h"
#include "../Debug.h"

#include <log4cpp/Category.hh>
#include <sstream>

using namespace Supervisor;

/////////////////////////////////////////////////////////////////////////////
Server::Server(const PdServ::Main *main, const PdServ::Config &config):
    PdServ::Session(main),
    main(main),
    eventLog(log4cpp::Category::getInstance(
                std::string(main->name).append(".event").c_str())),
    mutex(1)
{
    start();
}

/////////////////////////////////////////////////////////////////////////////
Server::~Server()
{
}

/////////////////////////////////////////////////////////////////////////////
void Server::run()
{
    const PdServ::Event* event;
    bool state;
    struct timespec t;
    size_t index;

    while (true) {
        ost::Thread::sleep(100);

        while ((event = main->getNextEvent(this, &index, &state, &t))) {

            std::ostringstream os;
            os << event->path << '[' << index << "]=" << state;

            log4cpp::Priority::Value prio;
            switch (event->priority) {
                case PdServ::Event::Emergency:
                    prio = log4cpp::Priority::FATAL;
                    break;
                case PdServ::Event::Alert:
                    prio = log4cpp::Priority::ALERT;
                    break;
                case PdServ::Event::Critical:
                    prio = log4cpp::Priority::CRIT;
                    break;
                case PdServ::Event::Error:
                    prio = log4cpp::Priority::ERROR;
                    break;
                case PdServ::Event::Warning:
                    prio = log4cpp::Priority::WARN;
                    break;
                case PdServ::Event::Notice:
                    prio = log4cpp::Priority::NOTICE;
                    break;
                case PdServ::Event::Info:
                    prio = log4cpp::Priority::INFO;
                    break;
                case PdServ::Event::Debug:
                    prio = log4cpp::Priority::DEBUG;
                    break;
            }

            if (state) {
                if (event->message)
                    eventLog.log(prio,
                            os.str() + ' ' + event->message[index]);
                else
                    eventLog.log(prio, os.str());
            }
            else
                eventLog.debug(os.str());
        }
    }
}
