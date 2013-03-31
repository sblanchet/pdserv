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
#include "../Debug.h"

#include <daemon.h>
#include <sstream>
#include <iostream>
#include <cerrno>
#include <cstring>
#include <sys/types.h>          // wait()
#include <sys/wait.h>           // wait()
#include <sys/ioctl.h>          // ioctl()
#include <fcntl.h>              // open()
#include <unistd.h>             // getopt()
#include <signal.h>             // getopt()
#include <cstdlib>              // atoi()

#include <log4cplus/consoleappender.h>
#include <log4cplus/syslogappender.h>
#include <log4cplus/layout.h>
#include <log4cplus/configurator.h>

#include "Main.h"
#include "../Config.h"

int debug = 3;
pid_t app[MAX_APPS];

const char *device_node = "/dev/etl";

/////////////////////////////////////////////////////////////////////////////
void cleanup(int,void*)
{
    ::daemon_pid_file_remove();
}

/////////////////////////////////////////////////////////////////////////////
void sighandler(int signal)
{
    int status;
    pid_t pid;
    switch (signal) {
        case SIGCHLD:
            pid = ::wait(&status);
            if (pid > 0) {
                for (size_t i = 0; i < MAX_APPS; ++i) {
                    if (app[i] == pid)
                        app[i] = 0;
                }
            }
            log_debug("Child killed %i", status);
            break;

        case SIGUSR1:
            debug++;
            break;

        case SIGUSR2:
            if (debug)
                debug--;
            break;

        default:
            LOG4CPLUS_INFO(log4cplus::Logger::getRoot(),
                    LOG4CPLUS_TEXT("PID ") << getpid()
                    << LOG4CPLUS_TEXT(" received signal ") << signal
                    << LOG4CPLUS_TEXT(". Exiting."));

            ::kill(0, SIGTERM);
            ::exit(EXIT_FAILURE);
    }
}

/////////////////////////////////////////////////////////////////////////////
void printhelp(const char *exe, const std::string &default_config)
{
    std::cerr << "Usage: " << exe << std::endl
        << "\t-h \t\tHelp" << std::endl
        << "\t-f \t\tStay in foreground (do not fork)" << std::endl
        << "\t-k \t\tKill running daemon and exit" << std::endl
        << "\t-d[level] \tDebug Level (implies foreground)" << std::endl
        << "\t-c <file.conf> \tConfig file (default: "
        << default_config << ')' << std::endl;
}

/////////////////////////////////////////////////////////////////////////////
void parse_command_line (int argc, char **argv,
        bool &fg, std::string &configFile)
{
    std::string default_config = configFile;
    int opt;

    // Parse command line options
    while ((opt = ::getopt(argc, argv, "d::fkc:h")) != -1) {
        switch (opt) {
            case 'd':
                // debug
                fg = 1;
                debug = optarg ? ::atoi(optarg) : debug+1;
                break;

            case 'k':
                // Kill daemon
                if (daemon_pid_file_kill(SIGTERM)) {
                    std::cerr << "ERROR: No running process" << std::endl;
                    ::exit(EXIT_FAILURE);
                }
                else
                    ::exit(EXIT_SUCCESS);
                break;

            case 'f':
                // Foreground
                fg = 1;
                break;

            case 'c':
                if (::access(optarg, R_OK)) {
                    int rv = errno;
                    std::cerr << "Error: Cannot read configuration " << optarg
                        << ": " << strerror(rv) << std::endl;
                    ::exit(rv);
                }
                configFile = optarg;
                break;

            default:
                printhelp(daemon_pid_file_ident, default_config);
                ::exit (opt == 'h' ? EXIT_SUCCESS : EXIT_FAILURE);
                break;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
void console_logging(bool fg)
{
    if (!fg)
        return;

    log4cplus::SharedAppenderPtr cerr(new log4cplus::ConsoleAppender(true));
    cerr->setLayout(
            std::auto_ptr<log4cplus::Layout>(new log4cplus::SimpleLayout()));

    log4cplus::Logger::getRoot().addAppender(cerr);
}

/////////////////////////////////////////////////////////////////////////////
void syslog_logging()
{
    log4cplus::helpers::Properties p;
    p.setProperty(LOG4CPLUS_TEXT("ident"),
            LOG4CPLUS_TEXT("etherlab_buddy"));
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
void configure_logging(const PdServ::Config& config)
{
    if (!config) {
        syslog_logging();
        return;
    }


    typedef std::basic_istringstream<log4cplus::tchar> tistringstream;
    tistringstream is(LOG4CPLUS_STRING_TO_TSTRING(config.toString()));
    log4cplus::PropertyConfigurator(is).configure();
}

/////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
    std::string configFile;
    PdServ::Config config;
    pid_t pid;
    bool fg = 0;

    daemon_pid_file_ident = daemon_ident_from_argv0(argv[0]);

    // Set default configuration file
    configFile
        .append(QUOTE(SYSCONFDIR))
        .append("/pdserv.conf");

    parse_command_line(argc, argv, fg, configFile);

    bool configAccess = !::access(configFile.c_str(), R_OK);
    if (configAccess) {
        const char *err = config.load(configFile.c_str());
        if (err) {
            std::cerr << err << std::endl;
            exit(EXIT_FAILURE);
        }
        log_debug("Loaded configuration file %s", configFile.c_str());
    }
    else if (!::access(configFile.c_str(), F_OK)) {
        fprintf(stderr, "Configuration file %s is not readable",
                configFile.c_str());
    }

    // Get the logging configutation
    configure_logging(config["logging"]);
    console_logging(fg);

    log4cplus::Logger log = log4cplus::Logger::getRoot();

    if (configAccess)
        LOG4CPLUS_INFO(log,
                LOG4CPLUS_TEXT("Configuration file ")
                << LOG4CPLUS_STRING_TO_TSTRING(configFile));

    // Make sure no other instance is running
    if ((pid = daemon_pid_file_is_running()) > 0) {
        LOG4CPLUS_ERROR(log,
                LOG4CPLUS_TEXT("Process already running with pid " << pid));
        ::exit(EBUSY);
    }

    // Foreground or daemon
    if (!fg) {
        pid = daemon_fork();

        if (pid) {
            LOG4CPLUS_INFO(log,
                    LOG4CPLUS_TEXT("Daemon started with pid ") << pid);
            ::exit(EXIT_SUCCESS);
        }
    }

    // Install some signal handlers
    if (::signal(SIGTERM, sighandler) == SIG_ERR
            or ::signal(SIGHUP,  sighandler) == SIG_ERR
            or ::signal(SIGINT,  sighandler) == SIG_ERR
            or ::signal(SIGCHLD, sighandler) == SIG_ERR) {
        LOG4CPLUS_WARN(log,
                LOG4CPLUS_TEXT("Could not set signal handler: ")
                << LOG4CPLUS_C_STR_TO_TSTRING(strerror(errno)));
    }

    // Cleanup on normal exit
    ::on_exit(cleanup, 0);

    // Remove and create new pid files
    daemon_pid_file_remove();
    if (daemon_pid_file_create() and !fg) {
        LOG4CPLUS_WARN(log,
                LOG4CPLUS_TEXT("Could not create log file ")
                << LOG4CPLUS_C_STR_TO_TSTRING(daemon_pid_file_proc_default()));
    }

    std::string path;
    path = device_node;
    path.append(1, '0');
    int etl_main = ::open(path.c_str(), O_NONBLOCK | O_RDWR);
    if (etl_main < 0) {
        LOG4CPLUS_FATAL(log,
                LOG4CPLUS_TEXT("Could not open main device ")
                << LOG4CPLUS_STRING_TO_TSTRING(path));
        return errno;
    }
    log_debug("open(%s) = %i", path.c_str(), etl_main);

    do {
        uint32_t apps;

        ioctl(etl_main, RTAC_GET_ACTIVE_APPS, &apps);
        for (unsigned i = 0; i < MAX_APPS; ++i, apps >>= 1) {
            if ((apps & 0x01) and !app[i]) {
                // A new application started
                log_debug("New application instance %i", i);

                struct app_properties app_properties;
                std::string file;
                int fd;

                {
                    std::ostringstream os;
                    os << device_node << (i+1);
                    file = os.str();
                }

                // Wait until the file is available
                while (::access(file.c_str(), F_OK)) {
                    LOG4CPLUS_INFO(log,
                            LOG4CPLUS_TEXT("Could not access device file ")
                            << LOG4CPLUS_STRING_TO_TSTRING(file)
                            << LOG4CPLUS_TEXT("; waiting..."));
                    sleep(1);
                }

                // Open the application's file handle
                fd = ::open(file.c_str(), O_NONBLOCK | O_RDONLY);

                if (fd == -1) {
                    LOG4CPLUS_ERROR(log,
                            LOG4CPLUS_TEXT("Could not open device ")
                            << LOG4CPLUS_STRING_TO_TSTRING(file));
                    continue;
                }
                log_debug("open(%s) = %i", file.c_str(), fd);

                // Get the properties
                if (!::ioctl(fd, GET_APP_PROPERTIES, &app_properties)) {
                    app[i] = fork();
                    if (!app[i]) {
                        // Swap the file descriptors fd and etl_main
                        if (::dup2(fd, etl_main) != -1) {
                            ::close(fd);
                            fd = etl_main;
                        }

                        Main(app_properties, config, fd).serve();
                    }
                }
                else
                    LOG4CPLUS_ERROR(log, LOG4CPLUS_TEXT(
                                "Could not get process information for ")
                            << LOG4CPLUS_STRING_TO_TSTRING(file));
                ::close(fd);
            }
            else if (!(apps & 0x01) and app[i]) {
                LOG4CPLUS_INFO(log,
                        LOG4CPLUS_TEXT("Finished serving application pid ")
                        << app[i]);
                ::kill(app[i], SIGTERM);
                app[i] = 0;
            }
        }

        // Wait until the status of a child or that of the main changes
        int n;
        do {
            fd_set fds;

            FD_ZERO(&fds);
            FD_SET(etl_main, &fds);

            n = ::select(etl_main + 1, &fds, 0, 0, 0);
            log_debug("select(etl_main) = %i", n);
            if (n < 0 and errno != EINTR) {
                // Some error occurred in select()
                n = errno;

                // Kill all the children
                ::kill(0, SIGKILL);
                return n;
            }
        } while (n <= 0);
    } while (true);
}
