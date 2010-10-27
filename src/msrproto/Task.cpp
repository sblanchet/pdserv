/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "../Main.h"
#include "../Variable.h"
#include "../Parameter.h"
#include "../Signal.h"

#include "Session.h"
#include "Server.h"
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
void Session::Task::rmSignal(const HRTLab::Signal *signal)
{
    if (signal) {
        SubscribedSet::iterator it = subscribedSet.find(signal->index);
        if (it != subscribedSet.end()) {
            delete[] it->second.data_bptr;
            delete it->second.element;
            subscribedSet.erase(it);
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
    cout << __PRETTY_FUNCTION__ << ' ' << n << " new signals " << signals.size() << endl;

    const HRTLab::Main::SignalList& sl = session->main->getSignals();
    SignalList::iterator sit = signals.begin();
    size_t offset = 0;
    size_t i = 0;

    while (i != n or sit != signals.end()) {
        cout << "considering index " << sigIdx[i] << endl;
        if (i != n and sit != signals.end()
                and (*sit).signal->index == sigIdx[i]) {
            // Signal is already in the list
            cout << "   index already in list" << endl;
            offset += (*sit).signal->memSize;
            i++;
            sit++;
            continue;
        }
        cout << "   index not in list" << endl;

        size_t i2 = i;
        SignalList::iterator sit2 = sit;
        while (i2 != n or sit2 != signals.end()) {
            cout << __LINE__ << "   i2 = " << i2 << endl;
            if (sit == signals.end()) {
                cout << __LINE__ << endl;
                i2 = n;
            }
            else if (i2 != n) {
                cout << __LINE__ << ' ' << i2 << endl;
                if (sigIdx[i2] == (*sit).signal->index) {
                    cout << __LINE__ << ' ' << sigIdx[i2] << endl;
                    sit2 = sit;
                    break;
                }
                i2++;
                cout << __LINE__ << ' ' << i2 << endl;
            }

            if (i == n) {
                cout << __LINE__ << endl;
                sit2 = signals.end();
            }
            else if (sit2 != signals.end()) {
                cout << __LINE__ << endl;
                if (sigIdx[i] == (*sit2).signal->index) {
                    cout << __LINE__ << endl;
                    i2 = i;
                    break;
                }
                sit2++;
                cout << __LINE__ << endl;
            }
        }
        cout << "   i2 = " << i2 << endl;

        // Signals between i and i2 are new in the list
        cout << __LINE__ << " Inserting from : " << i << ' ' << i2 <<endl;
        while (i != i2) {
            SubscribedSet::iterator it =
                subscribedSet.find(sigIdx[i]);
            const HRTLab::Signal *signal = sl[sigIdx[i]];

            if (it == subscribedSet.end()) {
                // This signal is not required by us
                SignalData sd = { signal, 0, 0 };

                signals.insert(sit, sd);
            }
            else {
                cout << __LINE__ << " inserting = " << signal->path << endl;
                it->second.offset = offset;
                signals.insert(sit, it->second);
            }
            offset += signal->memSize;
            cout << __LINE__ << " offset = " << offset << endl;
            i++;
        }

        // Signals between sit and sit2 are removed from the list
        cout << __LINE__ << " Erasing : "<<  this << endl;
        signals.erase(sit, sit2);
        sit = sit2;
        cout << __LINE__ << " Erasing : "<<  this << endl;
    }
    cout << __LINE__ << __func__ << endl;
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
