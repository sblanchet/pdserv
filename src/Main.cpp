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
#include <cstdlib>
#include <syslog.h>
#include <log4cpp/Category.hh>

#include "Main.h"
#include "Task.h"
#include "Signal.h"
#include "ProcessParameter.h"
#include "Config.h"
//#include "etlproto/Server.h"
#include "msrproto/Server.h"
#include "supervisor/Server.h"

using namespace PdServ;

/////////////////////////////////////////////////////////////////////////////
Main::Main(const std::string& name, const std::string& version):
    name(name), version(version), mutex(1),
    parameterLog(log4cpp::Category::getInstance(
                std::string(name).append(".parameter").c_str()))
{
    msrproto = 0;
    supervisor = 0;
}

/////////////////////////////////////////////////////////////////////////////
Main::~Main()
{
    for (ProcessParameters::iterator it = parameters.begin();
            it != parameters.end(); ++it)
        delete *it;

    delete msrproto;
    delete supervisor;
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
const Task* Main::getTask(double sampleTime) const
{
    for (TaskList::const_iterator it = task.begin();
            it != task.end(); ++it)
        if ((*it)->sampleTime == sampleTime)
            return *it;

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
const Main::ProcessParameters& Main::getParameters() const
{
    return parameters;
}

/////////////////////////////////////////////////////////////////////////////
void Main::startServers(const Config& config)
{
    supervisor = new Supervisor::Server(this,
            config["defaults"], config[name]);

    msrproto = new MsrProto::Server(this,
            config["defaults"]["msr"], config[name]);

//    EtlProto::Server etlproto(this);

}

/////////////////////////////////////////////////////////////////////////////
int Main::setParameter(const Session *, const ProcessParameter *param,
        size_t offset, size_t count, const char *data) const
{
    int rv = param->setValue(data, offset, count);

    if (rv) {
        parameterLog.notice("Parameter change %s failed (%s)",
                param->path.c_str(), strerror(errno));
        return rv;
    }
    msrproto->parameterChanged(param, offset, count);

    std::ostringstream os;
    os << param->path;

    if (count < param->memSize) {
        os << '[' << offset;
        if (count > 1)
            os << ".." << (offset + count - 1);
        os << ']';
    }

    os << " = ";

    param->dtype.print(os, data - offset, data, data + count);

    parameterLog.notice(os.str());

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
void Main::poll( Session *session, const Signal * const *s,
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

