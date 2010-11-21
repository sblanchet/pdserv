/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "config.h"

#include "../Main.h"
#include "../Signal.h"

#include "XmlDoc.h"
#include "Task.h"

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
        activeSet.clear();
    }
}

/////////////////////////////////////////////////////////////////////////////
void Task::addSignal(const HRTLab::Signal *signal,
        bool event, unsigned int decimation, size_t blocksize,
        bool base64, size_t precision)
{
    SubscribedSet::iterator it = subscribedSet.find(signal->index);
    size_t offset = 0;
    if (it != subscribedSet.end()) {
        delete[] it->second.data_bptr;
        delete it->second.element;
        offset = it->second.offset;
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
        (base64 ? MsrXml::toBase64 : MsrXml::toCSV),
        precision,
        data,
        data,
        data + dataLen,
        offset
    };

    sd.element->setAttribute("c", signal->index);

    subscribedSet[signal->index] = sd;
}

/////////////////////////////////////////////////////////////////////////////
void Task::newVariableList(const HRTLab::Variable * const *varList, size_t n)
{
    // Since it is not (should not be!) possible that required signal 
    // is not transmitted any more, only need to check for new signals
    unsigned int offset = 0;
    for (const HRTLab::Variable * const *v = varList; v != varList + n; v++) {
        SubscribedSet::iterator it = subscribedSet.find((*v)->index);
        if (it != subscribedSet.end()) {
            activeSet[*v] = &(it->second);
            it->second.offset = offset;
        }

        offset += (*v)->memSize;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Task::newValues(MsrXml::Element *parent, size_t seqNo,
        const char *pdoData)
{
    for (ActiveSet::iterator it = activeSet.begin();
            it != activeSet.end(); it++) {

        SignalData *sd = it->second;

        if (sd->trigger and --sd->trigger)
            continue;

        const char *dataPtr = pdoData + sd->offset;

        if (!sd->event) {
            sd->trigger = sd->decimation;

            std::copy(dataPtr, dataPtr + sd->sigMemSize, sd->data_pptr);
            sd->data_pptr += sd->sigMemSize;
            if (sd->data_pptr == sd->data_eptr) {
                sd->element->setAttribute("d",
                        sd->print(sd->variable, sd->data_bptr,
                            sd->precision, sd->blocksize));
                parent->appendChild(sd->element);
                sd->data_pptr = sd->data_bptr;
            }
        }
        else if (!std::equal(sd->data_bptr, sd->data_eptr, dataPtr)) {
            sd->trigger = sd->decimation;

            std::copy(dataPtr, dataPtr + sd->sigMemSize, sd->data_bptr);
            sd->element->setAttribute("d",
                    sd->print(sd->variable, sd->data_bptr, sd->precision, 1));
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
