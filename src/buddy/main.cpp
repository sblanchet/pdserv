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

#include <cerrno>
#include <sys/ioctl.h>          // ioctl()
#include <fcntl.h>              // open()
//#include <cc++/cmdoptns.h>

#include "Main.h"

const char *device_node = "/dev/etl";

//ost::CommandOptionNoArg traditional(
//        "traditional", "t", "Traditional MSR", false);

/////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
    const char *args[argc];
    Main *app[MAX_APPS];
    std::string path;
    
    std::copy(argv, argv+argc, args);
    std::fill_n(app, MAX_APPS, reinterpret_cast<Main*>(0));

    path = device_node;
    path.append("0");

    int etl_main = ::open(path.c_str(), O_NONBLOCK | O_RDWR);
    if (etl_main < 0) {
        debug() << "could not open" << path;
        return errno;
    }
    debug() << "opened" << path << '=' << etl_main;

    // Ignore child signals
    if (SIG_ERR == signal(SIGCHLD, SIG_IGN))
        return errno;

    do {
        uint32_t apps;

        ioctl(etl_main, RTAC_GET_ACTIVE_APPS, &apps);
        for (unsigned i = 0; apps; i++, apps >>= 1) {
            if ((apps & 0x1) and !app[i]) {
                debug() << "new Application" << i;
                app[i] = new Main(argc, args, i, device_node);
            }
            else if (!(apps & 0x1) and app[i]) {
                debug() << "finished Application" << i;
                delete app[i];
                app[i] = 0;
            }
            sleep(2);
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
