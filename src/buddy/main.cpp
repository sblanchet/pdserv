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

int debug = 3;

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
    switch (signal) {
        case SIGCHLD:
            ::wait(&status);
            debug() << "Child killed" << status;
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
void printhelp(const char *exe)
{
    std::cerr << "Usage: " << exe << std::endl
        << "\t-h \t\tHelp" << std::endl
        << "\t-f \t\tStay in foreground (do not fork)" << std::endl
        << "\t-k \t\tKill running daemon and exit" << std::endl
        << "\t-d[level] \tDebug Level (implies foreground)" << std::endl
        << "\t-c <file.conf> \tConfig file (default: "
        << QUOTE(SYSCONFDIR) << '/' << exe << ".conf)"
        << std::endl;
}

/////////////////////////////////////////////////////////////////////////////
int new_process(int id)
{
    pid_t pid = fork();
    if (pid < 0)
        return pid;
    else if (pid)
        return pid;

    return ::close(0);
}

/////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
    pid_t pid;
    int opt;
    bool fg = 0;

    daemon_pid_file_ident = daemon_ident_from_argv0(argv[0]);

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
                debug() << "cofig file" << optarg;
                //configfile.push(optarg);
                break;

            default:
                printhelp(daemon_pid_file_ident);
                exit (opt == 'h' ? EXIT_SUCCESS : EXIT_FAILURE);
                break;
        }
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
    daemon_pid_file_create();

    Main *app[MAX_APPS];
    std::fill_n(app, MAX_APPS, reinterpret_cast<Main*>(0));

    std::string path;
    path = device_node;
    path.append("0");
    int etl_main = ::open(path.c_str(), O_NONBLOCK | O_RDWR);
    if (etl_main < 0) {
        debug() << "could not open" << path;
        return errno;
    }
    debug() << "opened" << path << '=' << etl_main;

    do {
        uint32_t apps;

        ioctl(etl_main, RTAC_GET_ACTIVE_APPS, &apps);
        for (unsigned i = 0; apps; i++, apps >>= 1) {
            if ((apps & 0x1) and !app[i]) {
                debug() << "new Application" << i;
                std::ostringstream os;
                os << device_node << (i+1);
                app[i] = new Main(os.str());
            }
            else if (!(apps & 0x1) and app[i]) {
                debug() << "finished Application" << i;
                delete app[i];
                app[i] = 0;
            }
            sleep(1);
        }

        int n;
        do {
            fd_set fds;

            FD_ZERO(&fds);
            FD_SET(etl_main, &fds);

            n = ::select(etl_main + 1, &fds, 0, 0, 0);
            debug() << "select()" << n;
            if (n < 0) {
                // Some error occurred in select()
                n = errno;
                ::kill(0, SIGKILL);
                return n;
            }
            else if (!n) {
                // Signal was caught, check whether the children are still there
                //::kill(pid, 0);     // Test existence of pid
                debug() << "caught signal";
            }
        } while (!n);
    } while (true);
}
