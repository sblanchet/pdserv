/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef MSRTASK_H
#define MSRTASK_H

namespace HRTLab {
    class Signal;
    class Variable;
    class Parameter;
    class Main;
}

namespace MsrXml {
    class Element;
}

namespace MsrProto {

class Session;

class Task {
    public:
        Task(Session *, HRTLab::Main *);
        ~Task();

        void setQuiet(bool);

        void rmSignal(const HRTLab::Signal *s);
        void addSignal(const HRTLab::Signal *s,
                unsigned int decimation, size_t blocksize,
                bool base64, size_t precision);
        void newList(unsigned int *tasks, size_t n);
        void newValues(MsrXml::Element *, const char*);

    private:
        Session * const session;
        HRTLab::Main * const main;

        bool quiet;

        struct SignalData {
            const HRTLab::Signal *signal;
            MsrXml::Element *element;
            unsigned int decimation;
            unsigned int trigger;
            size_t blocksize;
            size_t sigMemSize;
            std::string (*print)(const HRTLab::Variable *v,
                    const char* data, size_t precision, size_t n);
            size_t precision;
            char *data_bptr;
            char *data_pptr;
            char *data_eptr;
            size_t offset;
        };

        typedef std::list<SignalData> SignalList;
        SignalList signals;

        typedef std::map<unsigned int, SignalData> SubscribedSet;
        SubscribedSet subscribedSet;
};

}
#endif //MSRTASK_H