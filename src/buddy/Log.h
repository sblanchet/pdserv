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

#ifndef LOGGER_H
#define LOGGER_H

#include <log4cpp/FileAppender.hh>
#include <log4cpp/SyslogAppender.hh>

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class AppenderPriority {
    public:
        log4cpp::Priority::Value getPriority() const;
        void setPriority( const std::string& prioString,
                log4cpp::Priority::Value defaultPrio);

    protected:
        AppenderPriority();

        bool accept(const log4cpp::LoggingEvent &event);

    private:
        log4cpp::Priority::Value priority;
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
struct FileAppender: public log4cpp::FileAppender, public AppenderPriority {
    FileAppender (const std::string &name, const std::string &fileName);
    void _append (const log4cpp::LoggingEvent &event);
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
struct SyslogAppender:
    public log4cpp::SyslogAppender, public AppenderPriority {
        SyslogAppender (const std::string &name,
                const std::string &syslogName, int facility);
        void _append (const log4cpp::LoggingEvent &event);
};

#endif // LOGGER_H
