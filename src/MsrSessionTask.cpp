/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "MsrSession.h"
#include "Main.h"
#include "Variable.h"
#include "Parameter.h"
#include "Signal.h"
#include "MsrServer.h"
#include "XmlDoc.h"

#include <iostream>
#include <limits>

#include <iomanip>
using std::cout;
using std::cerr;
using std::endl;

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Session::Task::Task(Session *s): session(s)
{
    quiet = false;
}

/////////////////////////////////////////////////////////////////////////////
Session::Task::~Task()
{
    for (SubscribedSet::const_iterator it = subscribedSet.begin();
            it != subscribedSet.end(); it++) {
        delete[] it->second.data_bptr;
        delete it->second.element;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::Task::addSignal(const HRTLab::Signal *signal,
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
        (base64 ? toBase64 : toCSV),
        precision
    };
    sd.data_bptr = new char[blocksize * signal->memSize];
    sd.data_pptr = sd.data_bptr;
    sd.data_eptr = sd.data_bptr + blocksize * signal->memSize;

    sd.element->setAttribute("c", signal->index);

    subscribedSet[signal->index] = sd;

    if (!quiet && session->quiet) {
        quiet = true;
        for (SignalList::iterator it = signals.begin();
                it != signals.end(); it++) {
            (*it).data_pptr = (*it).data_bptr;
            (*it).trigger = (*it).decimation;
        }
    }

    quiet = session->quiet;
}

/////////////////////////////////////////////////////////////////////////////
void Session::Task::newList(unsigned int *sigIdx, size_t n)
{
    cout << __PRETTY_FUNCTION__ << endl;

    const HRTLab::Main::SignalList& sl = session->main->getSignals();
    SignalList::iterator sit = signals.begin();
    size_t offset = 0;

    for (size_t i = 0; i < n; i++) {
        if (sit != signals.end() and (*sit).signal->index == sigIdx[i]) {
            // Signal is already in the list
            sit++;
            offset += (*sit).signal->memSize;
            continue;
        }

        size_t i2 = i + 1;
        SignalList::iterator sit2 = sit;
        sit2++;
        bool insert = false, remove = false;
        while (i2 != n and sit2 != signals.end()) {
            if (sigIdx[i2++] == (*sit).signal->index) {
                // Signals between i and i2 are new
                insert = true;
                break;
            }

            if (sigIdx[i] == (*sit2++).signal->index) {
                remove = true;
                break;
            }
        }

        if (sit2 == signals.end() and i2 == n) {
            insert = true;
            remove = true;
        }
        else if (i2 == n) {
            while (sit2 != signals.end()) {
                if (sigIdx[i] == (*sit2).signal->index)
                    break;
                sit++;
            }
            remove = true;
        }
        else {
            while (i2 != n) {
                if (sigIdx[i2] == (*sit).signal->index)
                    break;
                i2++;
            }
            insert = true;
        }

        if (insert) {
            // Signals between i and i2 are new in the list
            do {
                SubscribedSet::iterator it =
                    subscribedSet.find(sigIdx[i]);
                const HRTLab::Signal *signal = sl[sigIdx[i]];

                if (it == subscribedSet.end()) {
                    // This signal is not required by us
                    SignalData sd = { signal, 0, 0 };

                    signals.insert(++sit, sd);
                }
                else {
                    it->second.offset = offset;
                    signals.insert(sit, it->second);
                }
                offset += signal->memSize;
            } while (++i != i2);
        }

        if (remove) {
            // Signals between sit and sit2 are removed from the list
            signals.erase(sit, sit2);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::Task::newValues(MsrXml::Element *parent, const char* dataPtr)
{
    if (quiet)
        return;

    for (SignalList::iterator it = signals.begin();
            it != signals.end(); it++) {

        SignalData& sd = *it;

        if (sd.decimation) {
            if (!--sd.trigger) {
                sd.trigger = sd.decimation;

                std::copy(dataPtr, dataPtr + sd.sigMemSize, sd.data_pptr);
                sd.data_pptr += sd.sigMemSize;
                if (sd.data_pptr == sd.data_eptr) {
                    sd.element->setAttribute("d",
                            sd.print(sd.signal, sd.data_bptr,
                                sd.precision, sd.blocksize));
                    parent->appendChild(sd.element);
                    sd.data_pptr = sd.data_bptr;
                }
            }
        }
        else if (sd.trigger) {
            // Event triggering
            if (!std::equal(sd.data_bptr, sd.data_eptr, dataPtr)) {
                std::copy(dataPtr, dataPtr + sd.sigMemSize, sd.data_bptr);
                sd.element->setAttribute("d",
                        sd.print(sd.signal, sd.data_bptr, sd.precision, 1));
                parent->appendChild(sd.element);
            }
        }
        dataPtr += sd.signal->width;
    }
}
