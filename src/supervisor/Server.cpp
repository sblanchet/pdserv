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

#include "config.h"

#include "Server.h"
#include "../Main.h"
#include "../Event.h"
#include "../Debug.h"

#include <log4cplus/logger.h>
#include <sstream>
#include <cstdio>

using namespace Supervisor;

/////////////////////////////////////////////////////////////////////////////
Server::Server(const PdServ::Main *main, const PdServ::Config &config):
    PdServ::Session(main),
    main(main),
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
    const log4cplus::Logger eventLog(log4cplus::Logger::getInstance(
                LOG4CPLUS_STRING_TO_TSTRING("event")));

    while (true) {
        ost::Thread::sleep(100);

        while ((event = main->getNextEvent(this, &index, &state, &t))) {
            char time[50];
            ::snprintf(time, sizeof(time), "%zu.%09lu", t.tv_sec, t.tv_nsec);

            log4cplus::tostringstream os;
            os << LOG4CPLUS_C_STR_TO_TSTRING(state ? "Set " : "Reset ")
                << LOG4CPLUS_STRING_TO_TSTRING(event->path)
                << '[' << index << ']' << '@' << time;

            log4cplus::LogLevel prio;
            switch (event->priority) {
                case PdServ::Event::Emergency:
                case PdServ::Event::Alert:
                case PdServ::Event::Critical:
                    prio = log4cplus::FATAL_LOG_LEVEL;
                    break;
                case PdServ::Event::Error:
                    prio = log4cplus::ERROR_LOG_LEVEL;
                    break;
                case PdServ::Event::Warning:
                    prio = log4cplus::WARN_LOG_LEVEL;
                    break;
                case PdServ::Event::Notice:
                    break;
                case PdServ::Event::Info:
                    prio = log4cplus::INFO_LOG_LEVEL;
                    break;
                case PdServ::Event::Debug:
                    prio = log4cplus::DEBUG_LOG_LEVEL;
                    break;
            }

            if (state) {
                if (event->message and event->message[index])
                    os << ' '
                        << LOG4CPLUS_C_STR_TO_TSTRING(event->message[index]);

                eventLog.log(prio, os.str());
            }
            else
                LOG4CPLUS_DEBUG(eventLog, os.str());
        }
    }
}
