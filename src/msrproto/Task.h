/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef MSRTASK_H
#define MSRTASK_H

namespace HRTLab {
    class Main;
    class Receiver;
    class Signal;
    class Parameter;
}

namespace MsrXml {
    class Element;
}

namespace MsrProto {

class Session;

class Task {
    public:
        Task(Session *);
        ~Task();

        void rmSignal(const HRTLab::Signal *s);
        void addSignal(const HRTLab::Signal *s,
                bool event, unsigned int decimation, size_t blocksize,
                bool base64, size_t precision);

        void process(MsrXml::Element *, const HRTLab::Receiver&);
        void newSignalList(const HRTLab::Receiver&);
        void newValues(MsrXml::Element *, const HRTLab::Receiver&);

        void sync();

    private:
        Session * const session;

        struct SignalData {
            const HRTLab::Signal *signal;
            MsrXml::Element *element;
            bool event;
            unsigned int decimation;
            unsigned int trigger;
            size_t blocksize;
            size_t sigMemSize;
            void (MsrXml::Element::*printValue)(const char *,
                    const HRTLab::Variable *v, const char* data,
                    size_t precision, size_t n);
            size_t precision;
            char *data_bptr;
            char *data_pptr;
            char *data_eptr;
        };

        typedef std::map<const HRTLab::Signal *, SignalData*> ActiveSet;
        ActiveSet activeSet;

        typedef std::map<const HRTLab::Signal*, SignalData> SubscribedSet;
        SubscribedSet subscribedSet;
};

}
#endif //MSRTASK_H
