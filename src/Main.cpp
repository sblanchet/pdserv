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
template<class T>
T* ptr_align(void *p)
{
    const size_t mask = sizeof(T*) - 1;
    return reinterpret_cast<T*>(((unsigned)p + mask) & ~mask);
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
int Main::newParameter(const Parameter *p)
{
    if (variableMap.find(p->path) != variableMap.end())
        return -EEXIST;

    parameters.push_back(p);
    variableMap[p->path] = p;

    return 0;
}
