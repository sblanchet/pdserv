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

#include <cerrno>

#include "Main.h"
#include "Signal.h"
#include "Parameter.h"
//#include "etlproto/Server.h"
#include "msrproto/Server.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace PdServ;

/////////////////////////////////////////////////////////////////////////////
Main::Main(int argc, const char *argv[], const char *name, const char *version):
    name(name), version(version), argc(argc), argv(argv), mutex(1)
{
    msrproto = 0;
}

/////////////////////////////////////////////////////////////////////////////
Main::~Main()
{
    delete msrproto;

    for (VariableMap::const_iterator it = variableMap.begin();
            it != variableMap.end(); it++)
        delete it->second;
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
int Main::newSignal(Signal *s)
{
    if (variableMap.find(s->path) != variableMap.end())
        return -EEXIST;

    signals.push_back(s);
    variableMap[s->path] = s;

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
const Main::Signals& Main::getSignals() const
{
    return signals;
}

/////////////////////////////////////////////////////////////////////////////
const Main::Parameters& Main::getParameters() const
{
    return parameters;
}

/////////////////////////////////////////////////////////////////////////////
int Main::newParameter(Parameter *p)
{
    if (variableMap.find(p->path) != variableMap.end())
        return -EEXIST;

    parameters.push_back(p);
    variableMap[p->path] = p;

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
int Main::startProtocols()
{
    msrproto = new MsrProto::Server(this, argc, argv);

//    EtlProto::Server etlproto(this);

//    etlproto.start();
    msrproto->start();

//    etlproto.join();
    msrproto->join();

    cout << "XXXXXXXXXXXXXXXXXXXXXX" << endl;
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
void Main::parameterChanged(const Parameter *p, size_t startIndex,
                size_t nelem) const
{
    msrproto->parameterChanged(p, startIndex, nelem);
}

/////////////////////////////////////////////////////////////////////////////
void Main::poll( Session *session,
        const Signal * const *s, size_t nelem, char *buf) const
{
    ost::SemaphoreLock lock(mutex);

    for (size_t i = 0; i < nelem; ++i) {
        s[i]->poll(session, buf);
        buf += s[i]->memSize;
    }

    processPoll();
}

