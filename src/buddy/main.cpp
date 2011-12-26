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
            cerr_debug() << "Child killed" << status;
            break;

        case SIGUSR1:
            debug++;
            break;

        case SIGUSR2:
            if (debug)
                debug--;
            break;

        default:
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

    if (!::access(configFile.c_str(), R_OK)) {
        const char *err = config.load(configFile.c_str());
        if (err) {
            std::cerr << err << std::endl;
            exit(EXIT_FAILURE);
        }
        cerr_debug() << "load config" << configFile;
    }
    
    // Make sure no other instance is running
    if ((pid = daemon_pid_file_is_running()) > 0) {
        std::cerr
            << "ERROR: Process already running with pid " << pid << std::endl;
        ::exit(EBUSY);
    }

    // Foreground or daemon
    if (!fg) {
        pid = daemon_fork();

        if (pid) {
            ::exit(EXIT_SUCCESS);
        }
    }

    // Install some signal handlers
    if (::signal(SIGUSR1, sighandler) == SIG_ERR
            or ::signal(SIGUSR2, sighandler) == SIG_ERR
            or ::signal(SIGTERM, sighandler) == SIG_ERR
            or ::signal(SIGHUP,  sighandler) == SIG_ERR
            or ::signal(SIGINT,  sighandler) == SIG_ERR
            or ::signal(SIGCHLD, sighandler) == SIG_ERR) {
        std::cerr << "Could not set signal handler: "
            << strerror(errno) << std::endl;
    }

    // Cleanup on normal exit
    ::on_exit(cleanup, 0);

    // Remove and create new pid files
    daemon_pid_file_remove();
    if (daemon_pid_file_create() and !fg) {
        // FIXME log this
    }

    std::string path;
    path = device_node;
    path.append("0");
    int etl_main = ::open(path.c_str(), O_NONBLOCK | O_RDWR);
    if (etl_main < 0) {
        cerr_debug() << "could not open" << path;
        return errno;
    }
    cerr_debug() << "opened" << path << '=' << etl_main;

    do {
        uint32_t apps;

        ioctl(etl_main, RTAC_GET_ACTIVE_APPS, &apps);
        for (unsigned i = 0; i < MAX_APPS; ++i, apps >>= 1) {
            pid_t pid = app[i];
            if ((apps & 0x01) and !pid) {
                // A new application started
                cerr_debug() << "new Application" << i;

                struct app_properties app_properties;
                int fd;

                {
                    std::ostringstream file;
                    file << device_node << (i+1);

                    while (::access(file.str().c_str(), F_OK))
                        sleep(1);

                    // Open the application's file handle
                    fd = ::open(file.str().c_str(), O_NONBLOCK | O_RDONLY);
                }

                if (fd > 0) {
                    // Get the properties
                    if (!::ioctl(fd, GET_APP_PROPERTIES, &app_properties)) {
                        app[i] = fork();
                        if (!app[i]) {
                            ::close(etl_main);

                            // Swap the file descriptors fd and etl_main
                            if (::dup2(fd, etl_main) != -1) {
                                ::close(fd);
                                fd = etl_main;
                            }

                            Main().serve(config[app_properties.name],
                                    fd, &app_properties);
                        }
                    }
                    ::close(fd);
                }
            }
            else if (!(apps & 0x01) and pid) {
                cerr_debug() << "finished Application" << i;
                ::kill(pid, SIGTERM);
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
            cerr_debug() << "select()" << n;
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
