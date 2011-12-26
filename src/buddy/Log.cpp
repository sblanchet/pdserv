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

#include <log4cpp/SimpleLayout.hh>
#include <log4cpp/PatternLayout.hh>

#include "Log.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
AppenderPriority::AppenderPriority()
{
    priority = log4cpp::Priority::NOTSET;
}

/////////////////////////////////////////////////////////////////////////////
log4cpp::Priority::Value AppenderPriority::getPriority() const
{
    return priority;
}

/////////////////////////////////////////////////////////////////////////////
void AppenderPriority::setPriority( const std::string& prioString,
        log4cpp::Priority::Value defaultPrio)
{
    try {
        priority = log4cpp::Priority::getPriorityValue(prioString);
    }
    catch (std::invalid_argument&) {
        priority = log4cpp::Priority::WARN;
    }
}

/////////////////////////////////////////////////////////////////////////////
void AppenderPriority::accept(const log4cpp::LoggingEvent &event)
{
    if (event.priority <= priority) {
        log4cpp::threading::ScopedLock lock(mutex);
        log(event);
    }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
FileAppender::FileAppender ( const std::string &name,
        const std::string &fileName): log4cpp::FileAppender(name, fileName)
{
    log4cpp::PatternLayout *layout = new log4cpp::PatternLayout;
    layout->setConversionPattern(
            log4cpp::PatternLayout::BASIC_CONVERSION_PATTERN);

    setLayout(layout);
}

/////////////////////////////////////////////////////////////////////////////
void FileAppender::_append (const log4cpp::LoggingEvent &event)
{
    accept(event);
}

/////////////////////////////////////////////////////////////////////////////
void FileAppender::log (const log4cpp::LoggingEvent &event)
{
    log4cpp::FileAppender::_append(event);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
SyslogAppender::SyslogAppender(
        const std::string &name, const std::string &syslogName, int facility):
    log4cpp::SyslogAppender(name, syslogName, facility)
{
    setLayout(new log4cpp::SimpleLayout());
}

/////////////////////////////////////////////////////////////////////////////
void SyslogAppender::_append (const log4cpp::LoggingEvent &event)
{
    accept(event);
}

/////////////////////////////////////////////////////////////////////////////
void SyslogAppender::log (const log4cpp::LoggingEvent &event)
{
    log4cpp::SyslogAppender::_append(event);
}
