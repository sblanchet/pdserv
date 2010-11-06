/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "PdoSignalList.h"
#include "Main.h"
#include "Signal.h"

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

using namespace HRTLab;

/////////////////////////////////////////////////////////////////////////////
PdoSignalList::PdoSignalList(Main *m): main(m)
{
}

/////////////////////////////////////////////////////////////////////////////
PdoSignalList::~PdoSignalList()
{
}

/////////////////////////////////////////////////////////////////////////////
const PdoSignalList& PdoSignalList::operator=(const PdoSignalList &o)
{
    return o;
}

/////////////////////////////////////////////////////////////////////////////
void PdoSignalList::newSignals(const unsigned int *idx, size_t n)
{
    size_t sigoffset = 0;
    const Main::SignalList& sl = main->getSignals();

    sigOffsetMap.clear();

    while (n--) {
        const Signal *s = sl[*idx++];
        sigOffsetMap[s] = sigoffset;
        sigoffset += s->memSize;
    }
}

/////////////////////////////////////////////////////////////////////////////
const PdoSignalList::SigOffsetMap& PdoSignalList::getSignalOffsetMap() const
{
    return sigOffsetMap;
}

/////////////////////////////////////////////////////////////////////////////
bool PdoSignalList::isActive(const Signal *s) const
{
    return sigOffsetMap.find(s) != sigOffsetMap.end();
}
