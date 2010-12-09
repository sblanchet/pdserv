/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "config.h"

#include "Session.h"
#include "Main.h"
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
    main(m), receiver(new Receiver*[main->nst])
{
    main->gettime(&connectedTime);

    inBytes = 0;
    outBytes = 0;

    for (unsigned int i = 0; i < main->nst; i++)
        receiver[i] = main->newReceiver(i);
}

/////////////////////////////////////////////////////////////////////////////
Session::~Session()
{
    main->unsubscribe(this);

    for (unsigned int i = 0; i < main->nst; i++)
        delete receiver[i];

    delete[] receiver;
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

/////////////////////////////////////////////////////////////////////////////
void Session::rxPdo()
{
    for (unsigned int i = 0; i < main->nst; i++)
        receiver[i]->process(this);
}

