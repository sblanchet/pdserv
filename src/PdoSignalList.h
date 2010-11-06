/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef PDOSIGNALLIST_H
#define PDOSIGNALLIST_H

#include <cstddef>
#include <map>
#include <set>

namespace HRTLab {

class Main;
class Signal;

class PdoSignalList {
    public:
        PdoSignalList(Main *);
        const PdoSignalList& operator=(const PdoSignalList& o);

        ~PdoSignalList();

        void newSignals(const unsigned int *id, size_t n);

        bool isActive(const Signal *s) const;

        typedef std::map<const Signal*, size_t> SigOffsetMap;
        const SigOffsetMap& getSignalOffsetMap() const;

    private:

        Main * const main;

        SigOffsetMap sigOffsetMap;
};

}
#endif //PDOSIGNALLIST_H
