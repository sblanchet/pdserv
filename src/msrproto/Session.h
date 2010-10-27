/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef MSRSESSION_H
#define MSRSESSION_H

// Version der MSR_LIBRARIES 
#define VERSION  6
#define PATCHLEVEL 0
#define SUBLEVEL  10


#define MSR_VERSION (((VERSION) << 16) + ((PATCHLEVEL) << 8) + (SUBLEVEL))

//Liste der Features der aktuellen rtlib-Version, wichtig, muß aktuell gehalten werden
//da der Testmanager sich auf die Features verläßt

#define MSR_FEATURES "pushparameters,binparameters,maschinehalt,eventchannels,statistics,pmtime,aic,messages"

/* pushparameters: Parameter werden vom Echtzeitprozess an den Userprozess gesendet bei Änderung
   binparameters: Parameter können Binär übertragen werden
   maschinehalt: Rechner kann durch Haltbefehl heruntergefahren werden
   eventchannels: Kanäle werden nur bei Änderung übertragen
   statistics: Statistische Informationen über die anderen verbundenen Echtzeitprozesses
   pmtime: Die Zeit der Änderungen eines Parameters wird vermerkt und übertragen
   aic: ansychrone Input Kanäle
   messages: Kanäle mit dem Attribut: "messagetyp" werden von der msr_lib überwacht und bei Änderung des Wertes als Klartextmeldung verschickt V:6.0.10

*/

#include "../Session.h"

#include <cc++/socketport.h> 
#include <iosfwd>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <list>
#include <functional>

#include "rtlab/etl_data_info.h"

namespace HRTLab {
    class Signal;
    class Variable;
    class Parameter;
}

namespace MsrXml {
    class Element;
}

namespace MsrProto {

class Server;

class Session: public ost::SocketPort, public HRTLab::Session,
    private std::streambuf, private std::ostream {
    public:
        Session(
                Server *s,
                ost::SocketService *ss,
                ost::TCPSocket &socket,
                HRTLab::Main *main);
        ~Session();

        void broadcast(Session *s, const MsrXml::Element &element);

        void parameterChanged(const HRTLab::Parameter*);

    private:

        Server * const server;

        // Reimplemented from streambuf
        int sync();
        int overflow(int c);
        std::streamsize xsputn ( const char * s, std::streamsize n );

        // Reimplemented from SocketPort
        void expired();
        void pending();
        void output();
        void disconnect();

        // Reimplemented from HRTLab::Session
        std::string getName() const;
        std::string getClientName() const;
        size_t getCountIn() const;
        size_t getCountOut() const;
        struct timespec getLoginTime() const;
        size_t dataIn;
        size_t dataOut;
        struct timespec loginTime;

        bool writeAccess;
        bool quiet;
        bool echoOn;
        std::string remote;
        std::string applicationname;

        // Output buffer management
        // Somewhat elusive, but makes writing to end of buffer and sending
        // data fast
        char *wbuf;      // Beginning of buffer
        char *wbufpptr;  // Beginning of output data
        char *wbufeptr;  // End of output data
        size_t wbuffree; // Free space between end of data till buffer end
        void checkwbuf(size_t n);       // Check for free space

        // Input buffer management
        class Inbuf {
            public:
                Inbuf();
                ~Inbuf();
                bool empty() const;
                void erase(const char *p);
                char *bptr() const;
                const char *eptr() const;
                char *rptr(size_t n = 0);
                size_t free() const;

            private:
                size_t bufLen;
                char *_bptr, *_eptr;
        };

        Inbuf inbuf;

        // Parser routines
        // Returning true within these routines will interrupt parsing
        // and wait for more characters
        void parseInput(char* &pos, const char *end);

        char *bptr, *pptr;
        enum ParseState {
            FindStart, GetCommand, GetToken, GetAttribute, GetQuote, GetValue,
        };
        ParseState parseState;
        const char *commandPtr, *attrName;
        char *attrValue;
        size_t commandLen, attrLen;
        char quote;
        class Attr {
            public:
                void clear();
                void insert(const char *name, size_t nameLen);
                void insert(const char *name, size_t nameLen,
                        char *attr, size_t attrLen);
                bool find(const char *name,
                        char * &value, size_t &valueLen);
                bool isEqual(const char *name, const char *s);
                bool isTrue(const char *name);
                bool getString(const char *name, std::string &s);
                bool getUnsigned(const char *name, unsigned int &i);
                bool getUnsignedList(const char *name,
                        std::list<unsigned int> &i);

                const std::string *id;

            private:
                std::string _id;
                
                struct AttrPtrs {
                    const char *name;
                    size_t nameLen;
                    char *value;
                    size_t valueLen;
                };

                typedef std::multimap<size_t,AttrPtrs> AttrMap;
                AttrMap attrMap;
        };
        Attr attr;

        void processCommand();

        void broadcast();
        void echo();
        void ping();
        void readChannel();
        void readParameter();
        void readParamValues();
        void readStatistics();
        void remoteHost();
        void writeParameter();
        void xsad();
        void xsod();

        class Task {
            public:
                Task(Session *);
                ~Task();

                void rmSignal(const HRTLab::Signal *s);
                void addSignal(const HRTLab::Signal *s,
                        unsigned int decimation, size_t blocksize,
                        bool base64, size_t precision);
                void newList(unsigned int *tasks, size_t n);
                void newValues(MsrXml::Element *, const char*);

            private:
                Session * const session;
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

        std::vector<Task*> task;

        unsigned int * const signal_ptr_start;
        unsigned int *signal_ptr;

        static const char *getDTypeName(const HRTLab::Variable*);

        void setParameterAttributes(MsrXml::Element *e,
                const HRTLab::Parameter *p, bool shortReply, bool hex);
        void setChannelAttributes(MsrXml::Element *e,
                const HRTLab::Signal *s, bool shortReply);
};

}
#endif //MSRSESSION_H
