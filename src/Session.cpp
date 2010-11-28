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
    main->gettime(&connectedTime);

    inBytes = 0;
    outBytes = 0;
}

/////////////////////////////////////////////////////////////////////////////
Session::~Session()
{
    main->closeSession(this);
}

/////////////////////////////////////////////////////////////////////////////
SessionStatistics Session::getStatistics() const
{
    SessionStatistics s;

    s.remote = remoteHost;
    s.client = client;
    s.countIn = inBytes;
    s.countOut = outBytes;
    s.connectedTime = connectedTime;

    return s;
}
