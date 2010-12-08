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
void Task::rmSignal(const HRTLab::Signal * const *signal, size_t n)
{
    if (!signal) {
        // Remove all signals

        n = subscribedSet.size();
        size_t i = 0;

        const HRTLab::Signal *signal[n];
        for (SubscribedSet::const_iterator it = subscribedSet.begin();
                it != subscribedSet.end(); it++)
            signal[i++] = it->first;
        return rmSignal(signal, subscribedSet.size());
    }

    for (; n--; signal++) {
        SubscribedSet::iterator it = subscribedSet.find(*signal);
        if (it == subscribedSet.end())
            continue;

        delete[] it->second.data_bptr;
        delete it->second.element;
        subscribedSet.erase(it);
        activeSet.erase(*signal);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Task::addSignal(const HRTLab::Signal *signal, unsigned int idx,
        bool event, bool sync, unsigned int decimation, size_t blocksize,
        bool base64, size_t precision)
{
//    cout << __PRETTY_FUNCTION__ << signal->path << endl;
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
        sync,
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
bool Task::newSignalList(const HRTLab::Signal * const *s, size_t n)
{
    bool sync = false;

    // Since it is not (should not be!) possible that required signal 
    // is not transmitted any more, only need to check for new signals
    for (; n--; s++) {
        SubscribedSet::iterator it = subscribedSet.find(*s);
        if (it == subscribedSet.end())
            continue;

        activeSet[*s] = &(it->second);
        if (it->second.sync) {
            sync = true;
            it->second.sync = false;
        }
    }


    return sync;
}

/////////////////////////////////////////////////////////////////////////////
void Task::newSignalValues(MsrXml::Element *parent,
        const HRTLab::Receiver &receiver)
{
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

