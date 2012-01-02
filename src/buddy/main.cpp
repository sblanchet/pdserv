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

#include <log4cpp/PropertyConfigurator.hh>
#include <log4cpp/Category.hh>
#include <log4cpp/SyslogAppender.hh>
#include <log4cpp/BasicLayout.hh>
#include <log4cpp/OstreamAppender.hh>
#include <log4cpp/OstreamAppender.hh>

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
            log4cpp::Category::getRoot().notice(
                    "%i Received signal %i; Exiting",
                    getpid(), signal);

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

    log4cpp::Appender *cerr =
        new log4cpp::OstreamAppender("console", &std::cerr);
    cerr->setLayout(new log4cpp::BasicLayout());

    log4cpp::Category::getRoot().addAppender(cerr);
}

/////////////////////////////////////////////////////////////////////////////
void configure_logging(bool fg, const PdServ::Config& config)
{
    if (!config) {
        log4cpp::Category& root = log4cpp::Category::getRoot();

        log4cpp::Appender *appender =
            new log4cpp::SyslogAppender("syslog", "etherlab_buddy", LOG_LOCAL0);
        appender->setLayout(new log4cpp::BasicLayout());

        root.addAppender(appender);
        root.setPriority(log4cpp::Priority::NOTICE);

        return;
    }

    std::string text = config;

    char filename[100];
    strcpy(filename, "/tmp/buddylog.conf-XXXXXX");
    int fd = mkstemp(filename);
    log_debug(filename);

    ::write(fd, text.c_str(), text.size());
    ::close(fd);

    log4cpp::PropertyConfigurator::configure(filename);

    ::unlink(filename);
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
        .append(1, '/')
        .append(daemon_pid_file_ident)
        .append(".conf");

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
    configure_logging(fg, config["logging"]);
    log4cpp::Category& log = log4cpp::Category::getRoot();
    console_logging(fg);

    if (configAccess)
        log.notice("Configuration file %s", configFile.c_str());

    // Make sure no other instance is running
    if ((pid = daemon_pid_file_is_running()) > 0) {
        log.crit("ERROR: Process already running with pid %i", pid);
        ::exit(EBUSY);
    }

    // Foreground or daemon
    if (!fg) {
        pid = daemon_fork();

        if (pid) {
            log.notice("Daemon started with pid %i", pid);
            ::exit(EXIT_SUCCESS);
        }
    }

    // Install some signal handlers
    if (::signal(SIGTERM, sighandler) == SIG_ERR
            or ::signal(SIGHUP,  sighandler) == SIG_ERR
            or ::signal(SIGINT,  sighandler) == SIG_ERR
            or ::signal(SIGCHLD, sighandler) == SIG_ERR) {
        log.warn("Could not set signal handler: %s (%i)",
                strerror(errno), errno);
    }

    // Cleanup on normal exit
    ::on_exit(cleanup, 0);

    // Remove and create new pid files
    daemon_pid_file_remove();
    if (daemon_pid_file_create() and !fg) {
        log.warn("Could not create log file %s",
                daemon_pid_file_proc_default());
    }

    std::string path;
    path = device_node;
    path.append(1, '0');
    int etl_main = ::open(path.c_str(), O_NONBLOCK | O_RDWR);
    if (etl_main < 0) {
        log.crit("Could not open main device %s", path.c_str());
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
                    log.info("Could not access device file %s; waiting...",
                            file.c_str());
                    sleep(1);
                }

                // Open the application's file handle
                fd = ::open(file.c_str(), O_NONBLOCK | O_RDONLY);

                if (fd == -1) {
                    log.error("Could not open device %s", file.c_str());
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

                        Main(app_properties).serve(
                                config[app_properties.name], fd);
                    }
                }
                else
                    log.error("Could not get process information for %s",
                            file.c_str());
                ::close(fd);
            }
            else if (!(apps & 0x01) and app[i]) {
                log.notice("Finished serving application pid %i", app[i]);
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
