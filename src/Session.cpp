/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "config.h"

#include "Session.h"
#include "Main.h"
#include "Signal.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace HRTLab;

/////////////////////////////////////////////////////////////////////////////
Session::Session(Main *m): main(m)
{
    main->newSession(this);
}

/////////////////////////////////////////////////////////////////////////////
Session::~Session()
{
    main->closeSession(this);
}

/////////////////////////////////////////////////////////////////////////////
void Session::getSessionStatistics(Main::SessionStatistics&) const
{
}

/////////////////////////////////////////////////////////////////////////////
void Session::newVariableList(const Task *, const Variable **,
                size_t n)
{
}

/////////////////////////////////////////////////////////////////////////////
void Session::newPdoData(const Task *, unsigned int seqNo,
                const struct timespec *t, const char *)
{
}
