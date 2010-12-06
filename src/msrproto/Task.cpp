/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "config.h"

#include "../Signal.h"
#include "../Receiver.h"

#include "XmlDoc.h"
#include "Task.h"
#include "Session.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Task::Task()
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
        SubscribedSet::iterator it = subscribedSet.find(signal);
        delete[] it->second.data_bptr;
        delete it->second.element;
        subscribedSet.erase(it);
        activeSet.erase(signal);
    }
    else {
        for (SubscribedSet::iterator it = subscribedSet.begin();
                it != subscribedSet.end(); it++)
            rmSignal(it->first);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Task::addSignal(const HRTLab::Signal *signal, unsigned int idx,
        bool event, unsigned int decimation, size_t blocksize,
        bool base64, size_t precision)
{
    SubscribedSet::iterator it = subscribedSet.find(signal);
    if (it != subscribedSet.end()) {
        delete[] it->second.data_bptr;
        delete it->second.element;
    }

    if (event)
        // Event triggering
        blocksize = 1;
    else if (!blocksize)
        blocksize = 1;

    size_t dataLen = blocksize * signal->memSize;
    char *data = new char[dataLen];
    SignalData sd = {
        signal,
        new MsrXml::Element("F"),
        event,
        decimation,
        0,      // trigger
        blocksize,
        signal->memSize,
        (base64
         ? &MsrXml::Element::base64ValueAttr
         : &MsrXml::Element::csvValueAttr),
        precision,
        data,
        data,
        data + dataLen
    };

    sd.element->setAttribute("c", idx);

    subscribedSet[signal] = sd;
}

/////////////////////////////////////////////////////////////////////////////
void Task::newSignalList(const HRTLab::Signal * const *s, size_t n)
{
    return;

    // Since it is not (should not be!) possible that required signal 
    // is not transmitted any more, only need to check for new signals
    for (unsigned i = 0; i < n; i++) {
        SubscribedSet::iterator it = subscribedSet.find(s[i]);
        if (it != subscribedSet.end())
            activeSet[s[i]] = &(it->second);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Task::newSignalValues(MsrXml::Element *parent,
        const HRTLab::Receiver &receiver)
{
    return;
    for (ActiveSet::iterator it = activeSet.begin();
            it != activeSet.end(); it++) {

        SignalData *sd = it->second;

        if (sd->trigger and --sd->trigger)
            continue;

        const char* dataBuf = receiver.getValue(sd->signal);

        if (!sd->event) {
            sd->trigger = sd->decimation;

            std::copy(dataBuf, dataBuf + sd->sigMemSize, sd->data_pptr);
            sd->data_pptr += sd->sigMemSize;
            if (sd->data_pptr == sd->data_eptr) {
                (sd->element->*(sd->printValue))("d", sd->signal,
                        sd->data_bptr, sd->precision, sd->blocksize);
                parent->appendChild(sd->element);
                sd->data_pptr = sd->data_bptr;
            }
        }
        else if (!std::equal(sd->data_bptr, sd->data_eptr, dataBuf)) {
            sd->trigger = sd->decimation;

            std::copy(dataBuf, dataBuf + sd->sigMemSize, sd->data_bptr);
            (sd->element->*(sd->printValue))("d", sd->signal,
                    sd->data_bptr, sd->precision, 1);
            parent->appendChild(sd->element);
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
