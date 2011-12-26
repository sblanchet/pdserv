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

#include "config.h"
#include "../Debug.h"

#include <sstream>
#include <cerrno>
#include <cstdlib>              // exit()
#include <sys/ioctl.h>          // ioctl()
#include <unistd.h>             // fork()
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/wait.h>           // waitpid()

#include <log4cpp/OstreamAppender.hh>
#include <log4cpp/SimpleLayout.hh>
#include <log4cpp/Category.hh>

#include "Main.h"
#include "Log.h"
#include "Task.h"
#include "Signal.h"
#include "Parameter.h"
#include "SessionShadow.h"
#include "../Config.h"
#include "../Session.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Main::Main(): paramMutex(1), parameterBuf(0)
{
    pid = ::getpid();
}

/////////////////////////////////////////////////////////////////////////////
void Main::serve(const PdServ::Config& config, int fd,
        const struct app_properties *app_properties)
{
    log4cpp::Category& log =
        log4cpp::Category::getInstance(app_properties->name);

    this->app_properties = app_properties;
    this->fd = fd;

    name = app_properties->name;
    version = app_properties->version;

    log.setPriority(log4cpp::Priority::NOTSET);
    setupLogging(log, config["log"]);

    log.noticeStream() << "starting with pid " << pid;

    // Get a copy of the parameters
    parameterBuf = new char[app_properties->rtP_size];
    if (ioctl(fd, GET_PARAM, parameterBuf))
        goto out;

    char path[app_properties->variable_path_len+1];
    struct signal_info si;
    si.path = path;
    si.path_buf_len = app_properties->variable_path_len + 1;
    for (size_t i = 0; i < app_properties->param_count; i++) {

        si.index = i;
        if (ioctl(fd, GET_PARAM_INFO, &si) or !si.dim[0])
            continue;

        Parameter *p = new Parameter( this, parameterBuf + si.offset,
                SignalInfo(app_properties->name, &si));
        parameters.push_back(p);
    }
    
    log.infoStream()
        << "loaded " << app_properties->param_count << " parameters";


    mainTask = new Task(this, 1.0e-6 * app_properties->sample_period);
    task.push_back(mainTask);
    for (size_t i = 0; i < app_properties->signal_count; i++) {

        si.index = i;
        if (ioctl(fd, GET_SIGNAL_INFO, &si) or !si.dim[0])
            continue;

        mainTask->addSignal( SignalInfo(app_properties->name, &si));
    }
    log.infoStream()
        << "loaded " << app_properties->signal_count << " signals";

    shmem = ::mmap(0, app_properties->rtB_size * app_properties->rtB_count,
            PROT_READ, MAP_PRIVATE, fd, 0);
    if (shmem == MAP_FAILED)
        goto out;

    photoAlbum = reinterpret_cast<const char *>(shmem);

    readPointer = ioctl(fd, RESET_BLOCKIO_RP);

    photoReady = new unsigned int[app_properties->rtB_count];
    std::fill_n(photoReady, app_properties->rtB_count, 0);

    startServers(config);

    do {
        fd_set fds;
        int n;

        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        n = ::select(fd + 1, &fds, 0, 0, 0);
        if (n < 0) {
            // Some error occurred in select()
            goto out;
        }
        else if (!n) {
            // Signal was caught, check whether the children are still there
            //::kill(pid, 0);     // Test existence of pid
            //debug() << "caught signal";
        }
        else {
            struct data_p data_p;
            ioctl(fd, GET_WRITE_PTRS, &data_p);

            if (data_p.wp == -ENOSPC) {
                readPointer = ioctl(fd, RESET_BLOCKIO_RP);
            }
            else {
                for (int i = readPointer; i < data_p.wp; ++i)
                    photoReady[i] = photoCount++;
                readPointer = ioctl(fd, SET_BLOCKIO_RP, data_p.wp);
            }
        }
    } while (true);
    
out:
    log.fatalStream() << "Failed: " << strerror(errno) << " (" << errno << ')';
    ::exit(errno);
}

/////////////////////////////////////////////////////////////////////////////
void Main::setupLogging(log4cpp::Category& log, 
        PdServ::Config const& config)
{
    if (!config)
        return;

    setupTTYLog(log);
    setupSyslog(log, config["syslog"]);
    setupFileLog(log, config["file"]);
}

/////////////////////////////////////////////////////////////////////////////
void Main::setupTTYLog(log4cpp::Category& log)
{
    if (!::isatty(STDERR_FILENO))
        return;

    log4cpp::Appender *appender =
        new log4cpp::OstreamAppender( "cerr", &std::cerr);
    appender->setLayout(new log4cpp::SimpleLayout());

    log.addAppender(appender);
    log.setPriority(log4cpp::Priority::DEBUG);
}

/////////////////////////////////////////////////////////////////////////////
void Main::setupSyslog(log4cpp::Category& log, 
        PdServ::Config const& config)
{
    if (!config)
        return;

    struct {
        const char *name;
        int value;
    } facility[] = {
        {"LOG_AUTH",        LOG_AUTHPRIV},
        {"LOG_AUTHPRIV",    LOG_AUTHPRIV},
        {"LOG_CRON",        LOG_CRON},
        {"LOG_DAEMON",      LOG_DAEMON},
        {"LOG_FTP",         LOG_FTP},
        {"LOG_KERN",        LOG_KERN},
        {"LOG_LOCAL0",      LOG_LOCAL0},
        {"LOG_LOCAL1",      LOG_LOCAL1},
        {"LOG_LOCAL2",      LOG_LOCAL2},
        {"LOG_LOCAL3",      LOG_LOCAL3},
        {"LOG_LOCAL4",      LOG_LOCAL4},
        {"LOG_LOCAL5",      LOG_LOCAL5},
        {"LOG_LOCAL6",      LOG_LOCAL6},
        {"LOG_LOCAL7",      LOG_LOCAL7},
        {"LOG_LPR",         LOG_LPR},
        {"LOG_MAIL",        LOG_MAIL},
        {"LOG_NEWS",        LOG_NEWS},
        {"LOG_SYSLOG",      LOG_SYSLOG},
        {"LOG_USER",        LOG_USER},
        {"LOG_UUCP",        LOG_UUCP},
        {0,                 LOG_LOCAL0},
    };

    std::string s(config["facility"]);
    size_t i = 0;
    while (facility[i].name and strcasecmp(facility[i].name, s.c_str()))
        ++i;

    SyslogAppender *appender = new SyslogAppender(
            "Syslog", name.c_str(), facility[i].value);
    appender->setPriority(config["priority"], log4cpp::Priority::NOTICE);

    log.addAppender(appender);
    if (log.getPriority() == log4cpp::Priority::NOTSET
            or log.getPriority() < appender->getPriority())
        log.setPriority(appender->getPriority());
}

/////////////////////////////////////////////////////////////////////////////
void Main::setupFileLog(log4cpp::Category& log, 
        PdServ::Config const& config)
{
    if (!config)
        return;

    std::string path = config["path"];
    FileAppender *appender = new FileAppender( "File", path.c_str());
    appender->setPriority(config["priority"], log4cpp::Priority::INFO);

    log.addAppender(appender);
    if (log.getPriority() == log4cpp::Priority::NOTSET
            or log.getPriority() < appender->getPriority())
        log.setPriority(appender->getPriority());
}

/////////////////////////////////////////////////////////////////////////////
void Main::processPoll(unsigned int delay_ms,
        const PdServ::Signal * const *s, size_t nelem,
        void * const * pollDest, struct timespec *t) const
{
    const char *data = photoAlbum + readPointer * app_properties->rtB_size;

    for (size_t i = 0; i < nelem; ++i) {
        const Signal *signal = dynamic_cast<const Signal *>(s[i]);
        if (signal)
            signal->info.read( pollDest[i], data + signal->offset);
    }
}

/////////////////////////////////////////////////////////////////////////////
int Main::gettime(struct timespec *ts) const
{
    struct timeval tv;
    if (gettimeofday(&tv, 0))
        return -1;

    ts->tv_sec = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000;

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
PdServ::SessionShadow *Main::newSession(PdServ::Session *session) const
{
    return new SessionShadow(session, mainTask, readPointer, photoReady,
            photoAlbum, app_properties);
}

/////////////////////////////////////////////////////////////////////////////
int Main::setParameter(const Parameter *p, size_t startIndex,
        size_t count, const char *data, struct timespec *mtime) const
{
    ost::SemaphoreLock lock(paramMutex);
    struct param_change delta;

    delta.rtP = parameterBuf;
    delta.pos = data - parameterBuf + startIndex * p->width;
    delta.len = count * p->width;                                                        
    delta.count = 0;
    gettime(mtime);

    return ioctl(fd, CHANGE_PARAM, &delta);
}
