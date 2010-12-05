/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "config.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "Main.h"
#include "Task.h"
#include "Signal.h"
#include "Parameter.h"
#include "Session.h"
#include "etlproto/Server.h"
#include "msrproto/Server.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace HRTLab;

/////////////////////////////////////////////////////////////////////////////
Main::Main( const char *name, const char *version,
        double baserate, unsigned int nst):
    name(name), version(version), baserate(baserate), nst(nst),
    task(new Task*[nst])
{
    msrproto = 0;
}

/////////////////////////////////////////////////////////////////////////////
Main::~Main()
{
    delete msrproto;

    for (Signals::const_iterator it = signals.begin();
            it != signals.end(); it++)
        delete *it;

    for (Parameters::const_iterator it = parameters.begin();
            it != parameters.end(); it++)
        delete *it;
}

/////////////////////////////////////////////////////////////////////////////
int Main::gettime(struct timespec* t) const
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
int Main::newSignal(const Signal *s)
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
void Main::delSignal(const Signal *s)
{
    variableMap.erase(s->path);
}

/////////////////////////////////////////////////////////////////////////////
void Main::delParameter(const Parameter *p)
{
    variableMap.erase(p->path);
}

/////////////////////////////////////////////////////////////////////////////
int Main::newParameter(const Parameter *p)
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
    msrproto = new MsrProto::Server(this);

//    EtlProto::Server etlproto(this);

//    etlproto.start();
    msrproto->start();

//    etlproto.join();
    msrproto->join();

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
void Main::parametersChanged(const Parameter * const *p,
                size_t nelem) const
{
//    msrproto->parametersChanged(p, nelem);
}

/////////////////////////////////////////////////////////////////////////////
const Signal *Main::getSignal(const std::string& path) const
{
    const VariableMap::const_iterator it(variableMap.find(path));
    return dynamic_cast<const Signal*>(
            it != variableMap.end() ? it->second : 0);
}

/////////////////////////////////////////////////////////////////////////////
const Parameter *Main::getParameter(const std::string& path) const
{
    const VariableMap::const_iterator it(variableMap.find(path));
    return dynamic_cast<const Parameter*>(
            it != variableMap.end() ? it->second : 0);
}
