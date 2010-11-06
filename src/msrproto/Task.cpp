/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "../Main.h"
#include "../Variable.h"
#include "../Parameter.h"
#include "../Signal.h"

#include "Session.h"
#include "PdoSignalList.h"
#include "Server.h"
#include "XmlDoc.h"
#include "Task.h"

#include <limits>
#include <algorithm>

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Task::Task(Session *s, HRTLab::Main *m): session(s), main(m)
{
}

/////////////////////////////////////////////////////////////////////////////
Task::~Task()
{
    for (SubscribedSet::const_iterator it = subscribedSet.begin();
            it != subscribedSet.end(); it++) {
        delete[] it->second.data_bptr;
        delete it->second.element;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Task::rmSignal(const HRTLab::Signal *signal)
{
    if (signal) {
        SubscribedSet::iterator it = subscribedSet.find(signal->index);
        if (it != subscribedSet.end()) {
            delete[] it->second.data_bptr;
            delete it->second.element;
            subscribedSet.erase(it);
            activeSet.erase(signal);
        }
    }
    else {
        for (SubscribedSet::iterator it = subscribedSet.begin();
                it != subscribedSet.end(); it++) {
            delete[] it->second.data_bptr;
            delete it->second.element;
        }
        subscribedSet.clear();
    }
}

/////////////////////////////////////////////////////////////////////////////
void Task::addSignal(const HRTLab::Signal *signal,
        unsigned int decimation, size_t blocksize, bool base64,
        size_t precision)
{
    SubscribedSet::iterator it = subscribedSet.find(signal->index);
    if (it != subscribedSet.end()) {
        delete[] it->second.data_bptr;
        delete it->second.element;
    }

    size_t trigger;
    if (decimation) {
        trigger = decimation;
    }
    else {
        // Event triggering
        trigger = 1;
        blocksize = 1;
    }

    SignalData sd = {
        signal,
        new MsrXml::Element("F"),
        decimation,
        trigger,
        blocksize,
        signal->memSize,
        (base64 ? MsrXml::toBase64 : MsrXml::toCSV),
        precision
    };
    sd.data_bptr = new char[blocksize * signal->memSize];
    sd.data_pptr = sd.data_bptr;
    sd.data_eptr = sd.data_bptr + blocksize * signal->memSize;

    sd.element->setAttribute("c", signal->index);

    subscribedSet[signal->index] = sd;

    if (session->isSignalActive(signal))
        activeSet[signal] = &subscribedSet[signal->index];
}


/////////////////////////////////////////////////////////////////////////////
void Task::newSignalMap( const HRTLab::PdoSignalList::SigOffsetMap &s)
{
    // Since it is not (should not!) possible that required signal 
    // is not transmitted any more, only need to check for new signals
    for (HRTLab::PdoSignalList::SigOffsetMap::const_iterator it = s.begin();
            it != s.end(); it++) {

        SubscribedSet::iterator x = subscribedSet.find((it->first)->index);
        if (x == subscribedSet.end())
            continue;

        if (activeSet.find(it->first) == activeSet.end())
            activeSet[it->first] = &(x->second);

        x->second.offset = it->second;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Task::newValues(MsrXml::Element *parent, size_t seqNo, const char *pdoData)
{
    for (ActiveSet::iterator it = activeSet.begin();
            it != activeSet.end(); it++) {

        SignalData *sd = it->second;
        const char *dataPtr = pdoData + sd->offset;

        if (sd->decimation) {
            if (!--sd->trigger) {
                sd->trigger = sd->decimation;

                std::copy(dataPtr, dataPtr + sd->sigMemSize, sd->data_pptr);
                sd->data_pptr += sd->sigMemSize;
                if (sd->data_pptr == sd->data_eptr) {
                    sd->element->setAttribute("d",
                            sd->print(sd->signal, sd->data_bptr,
                                sd->precision, sd->blocksize));
                    parent->appendChild(sd->element);
                    sd->data_pptr = sd->data_bptr;
                }
            }
        }
        else if (sd->trigger) {
            // Event triggering
            if (!std::equal(sd->data_bptr, sd->data_eptr, dataPtr)) {
                std::copy(dataPtr, dataPtr + sd->sigMemSize, sd->data_bptr);
                sd->element->setAttribute("d",
                        sd->print(sd->signal, sd->data_bptr, sd->precision, 1));
                parent->appendChild(sd->element);
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
void Task::sync()
{
    for (ActiveSet::iterator it = activeSet.begin();
            it != activeSet.end(); it++) {
        SignalData *sd = it->second;

        sd->trigger = sd->decimation;
        sd->data_pptr = sd->data_bptr;
    }
}
