/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "../Main.h"
#include "../Signal.h"
#include "../Parameter.h"

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
        cout << __func__ << " looking for " << signal->index << endl;
        cout << " available is:";
        for (SubscribedSet::const_iterator it = subscribedSet.begin();
                it != subscribedSet.end(); it++)
            cout << ' ' << it->first;
        cout << endl;
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
        if (!blocksize)
            blocksize = 1;
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
}

/////////////////////////////////////////////////////////////////////////////
void Task::newVariableList(const HRTLab::Variable **varList, size_t n)
{
    // Since it is not (should not be!) possible that required signal 
    // is not transmitted any more, only need to check for new signals
    unsigned int offset = 0;
    for (const HRTLab::Variable **v = varList; v != varList + n; v++) {

        SubscribedSet::iterator it = subscribedSet.find((*v)->index);
        if (it != subscribedSet.end()) {
            activeSet[*v] = &(it->second);
            it->second.offset = offset;
        }

        offset += (*v)->memSize;
    }
    cout << endl;
}

/////////////////////////////////////////////////////////////////////////////
void Task::newValues(MsrXml::Element *parent, size_t seqNo,
        const char *pdoData)
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
                            sd->print(sd->variable, sd->data_bptr,
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
                        sd->print(sd->variable, sd->data_bptr,
                            sd->precision, 1));
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
