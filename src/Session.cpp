/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "Session.h"
#include "Main.h"
#include "Signal.h"

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

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
std::string Session::getName() const
{
    return std::string();
}

/////////////////////////////////////////////////////////////////////////////
std::string Session::getClientName() const
{
    return std::string();
}

/////////////////////////////////////////////////////////////////////////////
size_t Session::getCountIn() const
{
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
size_t Session::getCountOut() const
{
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
struct timespec Session::getLoginTime() const
{
    struct timespec t = {0,0};
    return t;
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
