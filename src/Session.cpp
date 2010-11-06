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
    for (unsigned int i = 0; i != main->nst; i++)
        pdoSignalList.push_back(PdoSignalList(m));

    main->newSession(this);
    
    pdoBlockPtr =
        reinterpret_cast<void * const *>(main->getSignalPtrStart());
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
void Session::newSignalMap(unsigned int,
        const HRTLab::PdoSignalList::SigOffsetMap&)
{
}

/////////////////////////////////////////////////////////////////////////////
void Session::newPdoData(unsigned int, unsigned int,
        const struct timespec *, const char *)
{
}

/////////////////////////////////////////////////////////////////////////////
void Session::receivePdo()
{
    while (*pdoBlockPtr) {
        pdoBlockPtr = &*pdoBlockPtr;

        const unsigned int *i =
            reinterpret_cast<const unsigned int *>(pdoBlockPtr + 1);
        unsigned int messageType = i[0];
        unsigned int tid = i[1];

        switch (messageType) {
            case HRTLab::Main::SubscriptionList:
                // After this block, the signals are transmitted in the order
                // given by this list
                // Format:
                //          tid:    unsigned int; task id of the list
                //          count:  unsigned int; number of signals transmitted
                //          idx...: unsigned int; list of signal indices
                {
                    pdoSignalList[tid].newSignals(i + 3, i[2]);
                    newSignalMap(tid, pdoSignalList[tid].getSignalOffsetMap());
                }
                break;

            case HRTLab::Main::SubscriptionData:
                // Block of signal data
                // Format:
                //          tid:    unsigned int; task id of the list
                //          seqNo:  unsigned int; sequence number of block
                //          time:   struct timespec; current process time
                //          data:   signal data in the order presented by
                //                  SubscriptionList
                {
                    const struct timespec *t = 
                        reinterpret_cast<const struct timespec *>(i + 3);
                    newPdoData( tid, i[2], t,
                            reinterpret_cast<const char *>(t + 1));
                }
                break;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
bool Session::isSignalActive(const Signal *s) const
{
    return pdoSignalList[s->tid].isActive(s);
}
