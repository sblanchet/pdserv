/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "config.h"

#include "Session.h"
#include "Main.h"
#include "Signal.h"
#include "Receiver.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace HRTLab;

/////////////////////////////////////////////////////////////////////////////
Session::Session(Main *m):
    main(m), receiver(main->newReceiver(this))
{
    main->gettime(&connectedTime);

    inBytes = 0;
    outBytes = 0;
}

/////////////////////////////////////////////////////////////////////////////
Session::~Session()
{
    delete receiver;
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
