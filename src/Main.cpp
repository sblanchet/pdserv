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

#include "config.h"
#include "Debug.h"

#include <cerrno>
#include <unistd.h>     // fork(), getpid(), chdir, sysconf, close
#include <signal.h>     // signal()
#include <limits.h>     // _POSIX_OPEN_MAX
#include <sys/types.h>  // umask()
#include <sys/stat.h>   // umask()

#include <log4cplus/syslogappender.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/streams.h>
#include <log4cplus/configurator.h>
#include <log4cplus/loggingmacros.h>

#include "Main.h"
#include "Task.h"
#include "Signal.h"
#include "Parameter.h"
#include "Config.h"
//#include "etlproto/Server.h"
#include "msrproto/Server.h"
#include "supervisor/Server.h"

using namespace PdServ;

/////////////////////////////////////////////////////////////////////////////
Main::Main(const std::string& name, const std::string& version):
    name(name), version(version), mutex(1),
    parameterLog(log4cplus::Logger::getInstance(
                LOG4CPLUS_STRING_TO_TSTRING("parameter")))
{
    msrproto = 0;
    supervisor = 0;
}

/////////////////////////////////////////////////////////////////////////////
Main::~Main()
{
    for (TaskList::iterator it = task.begin(); it != task.end(); ++it)
        delete *it;

    for (Parameters::iterator it = parameters.begin();
            it != parameters.end(); ++it)
        delete *it;
}

// /////////////////////////////////////////////////////////////////////////////
// int Main::setupDaemon()
// {
//     // Go to root
// #ifndef PDS_DEBUG
//     ::chdir("/");
//     ::umask(0777);
// #endif
// 
//     // close all file handles
//     // sysconf() usually returns one more than max file handle
//     long int fd_max = ::sysconf(_SC_OPEN_MAX);
//     if (fd_max < _POSIX_OPEN_MAX)
//         fd_max = _POSIX_OPEN_MAX;
//     else if (fd_max > 100000)
//         fd_max = 512;   // no rediculous values please
//     while (fd_max--) {
// #ifdef PDS_DEBUG
//         if (fd_max == 2)
//             break;
// #endif
//         ::close(fd_max);
//     }
// 
// #ifndef PDS_DEBUG
//     // Reopen STDIN, STDOUT and STDERR
//     if (STDIN_FILENO == ::open("/dev/null", O_RDWR)) {
//         dup2(STDIN_FILENO, STDOUT_FILENO);
//         dup2(STDIN_FILENO, STDERR_FILENO);
// 
//         fcntl( STDIN_FILENO, F_SETFL, O_RDONLY);
//         fcntl(STDOUT_FILENO, F_SETFL, O_WRONLY);
//         fcntl(STDERR_FILENO, F_SETFL, O_WRONLY);
//     }
// #endif
// 
//     return 0;
// }

/////////////////////////////////////////////////////////////////////////////
void Main::setConfig(const Config& c)
{
    config = c;
}

/////////////////////////////////////////////////////////////////////////////
void Main::setupLogging()
{
#ifdef PDS_DEBUG
    consoleLogging();
#endif

    if (config["logging"]) {
        typedef std::basic_istringstream<log4cplus::tchar> tistringstream;
        tistringstream is(
                LOG4CPLUS_STRING_TO_TSTRING(config["logging"].toString()));
        log4cplus::PropertyConfigurator(is).configure();
    }
    else {
        syslogLogging();
    }

    LOG4CPLUS_INFO(log4cplus::Logger::getRoot(),
            LOG4CPLUS_TEXT("Started application ")
            << LOG4CPLUS_STRING_TO_TSTRING(name)
            << LOG4CPLUS_TEXT(", Version ")
            << LOG4CPLUS_STRING_TO_TSTRING(version));
}

/////////////////////////////////////////////////////////////////////////////
void Main::consoleLogging()
{
    log4cplus::BasicConfigurator::doConfigure();
//    log4cplus::SharedAppenderPtr cerr(new log4cplus::ConsoleAppender(true));
//    cerr->setLayout(
//        std::auto_ptr<log4cplus::Layout>(new log4cplus::PatternLayout(
//                LOG4CPLUS_TEXT("%D %p %c %x: %m"))));
//
//    log4cplus::Logger::getRoot().addAppender(cerr);
}

/////////////////////////////////////////////////////////////////////////////
void Main::syslogLogging()
{
    log4cplus::helpers::Properties p;
    p.setProperty(LOG4CPLUS_TEXT("ident"),
            LOG4CPLUS_STRING_TO_TSTRING(name));
    p.setProperty(LOG4CPLUS_TEXT("facility"),LOG4CPLUS_TEXT("local0"));

    log4cplus::SharedAppenderPtr appender( new log4cplus::SysLogAppender(p));
    appender->setLayout(
            std::auto_ptr<log4cplus::Layout>(new log4cplus::PatternLayout(
                    LOG4CPLUS_TEXT("%-5p %c <%x>: %m"))));

    log4cplus::Logger root = log4cplus::Logger::getRoot();
    root.addAppender(appender);
    root.setLogLevel(log4cplus::INFO_LOG_LEVEL);
}

/////////////////////////////////////////////////////////////////////////////
int Main::gettime(struct timespec* t) const
{
    return localtime(t);
}

/////////////////////////////////////////////////////////////////////////////
int Main::localtime(struct timespec* t)
{
    struct timeval tv;

    if (gettimeofday(&tv, 0))
        return errno;
    t->tv_sec = tv.tv_sec;
    t->tv_nsec = tv.tv_usec * 1000;

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
void Main::getSessionStatistics(std::list<SessionStatistics>& stats) const
{
    msrproto->getSessionStatistics(stats);
    //etlproto->getSessionStatistics(stats);
}

/////////////////////////////////////////////////////////////////////////////
size_t Main::numTasks() const
{
    return task.size();
}

/////////////////////////////////////////////////////////////////////////////
const Task* Main::getTask(size_t n) const
{
    return task[n];
}

/////////////////////////////////////////////////////////////////////////////
const Main::Parameters& Main::getParameters() const
{
    return parameters;
}

/////////////////////////////////////////////////////////////////////////////
void Main::startServers()
{
    LOG4CPLUS_INFO(log4cplus::Logger::getRoot(), "Starting servers");

    supervisor = new Supervisor::Server(this, config["supervisor"]);
    msrproto = new MsrProto::Server(this, config["msr"]);

//    EtlProto::Server etlproto(this);
}

/////////////////////////////////////////////////////////////////////////////
void Main::stopServers()
{
    delete msrproto;
    delete supervisor;

    LOG4CPLUS_INFO(log4cplus::Logger::getRoot(), "Shut down servers");
    log4cplus::Logger::shutdown();
}

/////////////////////////////////////////////////////////////////////////////
void Main::parameterChanged(const Session* /*session*/, const Parameter *param,
        const char* data, size_t offset, size_t count) const
{
    msrproto->parameterChanged(param, offset, count);

    if (!parameterLog.isEnabledFor(log4cplus::INFO_LOG_LEVEL))
        return;

    std::ostringstream os;
    os.imbue(std::locale::classic());
    os << param->path;

    if (count < param->memSize) {
        os << '[' << offset;
        if (count > 1)
            os << ".." << (offset + count - 1);
        os << ']';
    }

    os << " = ";

    param->dtype.print(os, data + offset, count);

    parameterLog.forcedLog(log4cplus::INFO_LOG_LEVEL,
            LOG4CPLUS_STRING_TO_TSTRING(os.str()));
}

/////////////////////////////////////////////////////////////////////////////
void Main::poll(const Session *session, const Signal * const *s,
        size_t nelem, void *buf, struct timespec *t) const
{
    ost::SemaphoreLock lock(mutex);
    char *dest = reinterpret_cast<char*>(buf);
    double delay = 0.01;
    void *pollDest[nelem];

    for (size_t i = 0; i < nelem; ++i) {
        delay = std::max(delay,
                std::min(0.1, s[i]->poll(session, dest, t)));
        pollDest[i] = dest;
        dest += s[i]->memSize;
    }

    processPoll(static_cast<unsigned>(delay * 1000 + 0.5),
		    s, nelem, pollDest, t);
}

