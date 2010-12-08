/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "config.h"

#include "../Main.h"
#include "../Task.h"
#include "../Parameter.h"
#include "../Signal.h"
#include "../Receiver.h"

#include "Session.h"
#include "Server.h"
#include "Task.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

#define MSR_R   0x01    /* Parameter is readable */
#define MSR_W   0x02    /* Parameter is writeable */
#define MSR_WOP 0x04    /* Parameter is writeable in real-time */
#define MSR_MONOTONIC 0x8 /* List must be monotonic */
#define MSR_S   0x10    /* Parameter must be saved by clients */
#define MSR_G   0x20    /* Gruppenbezeichnung (unused) */
#define MSR_AW  0x40    /* Writeable by admin only */
#define MSR_P   0x80    /* Persistant parameter, written to disk */
#define MSR_DEP 0x100   /* This parameter is an exerpt of another parameter.
                           Writing to this parameter also causes an update
                           notice for the encompassing parameter */
#define MSR_AIC 0x200   /* Asynchronous input channel */

/* ein paar Kombinationen */
#define MSR_RWS (MSR_R | MSR_W | MSR_S)
#define MSR_RP (MSR_R | MSR_P)


using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Session::Session( Server *s, ost::SocketService *ss,
        ost::TCPSocket &socket, HRTLab::Main *main):
    SocketPort(0, socket), HRTLab::Session(main),
    server(s), task(new Task*[main->nst]),
    dataTag("data"), outbuf(this), inbuf(this)
{
//    cout << __LINE__ << __PRETTY_FUNCTION__ << this << endl;

    // Setup some internal variables
    writeAccess = false;
    echoOn = false;
    quiet = false;
    dataTag.setAttribute("level", 0);

    // Create enough tasks
    for (size_t i = 0; i < main->nst; i++) {
        task[i] = new Task();
    }

    const HRTLab::Main::Signals& signals = main->getSignals();
    signalCount = signals.size();
    signal = new const HRTLab::Signal*[signalCount];
    for (unsigned idx = 0; idx < signalCount; idx++) {
        const HRTLab::Signal *s = signals[idx];
        variableIndexMap[s] = idx;
        signal[idx] = s;
    }

    const HRTLab::Main::Parameters& parameters = main->getParameters();
    parameterCount = parameters.size();
    parameter = new const HRTLab::Parameter*[parameterCount];
    for (unsigned idx = 0; idx < parameterCount; idx++) {
        const HRTLab::Parameter *p = parameters[idx];
        variableIndexMap[parameters[idx]] = idx;
        parameter[idx] = p;
    }

    // Non-blocking read() and write()
    setCompletion(false);

    attach(ss);

    // Greet the new client
    MsrXml::Element greeting("connected");
    greeting.setAttribute("name", "MSR");
    greeting.setAttribute("host", "localhost");
    greeting.setAttribute("version", MSR_VERSION);
    greeting.setAttribute("features", MSR_FEATURES);
    greeting.setAttribute("recievebufsize", 100000000);
    outbuf << greeting << std::flush;

    expired();
}

/////////////////////////////////////////////////////////////////////////////
Session::~Session()
{
//    cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    server->sessionClosed(this);
    for (size_t i = 0; i < main->nst; i++) {
        delete task[i];
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::broadcast(Session *s, const MsrXml::Element &element)
{
    outbuf << element << std::flush;
}

/////////////////////////////////////////////////////////////////////////////
void Session::parameterChanged(const HRTLab::Parameter * const *p, size_t n) 
{
    MsrXml::Element pu("pu");
    while (n--) {
        if (*p) {
//            pu.setAttribute("index", variableIndex[*p++]);
            outbuf << pu << std::flush;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
size_t Session::getVariableIndex(const HRTLab::Variable *v) const
{
    return variableIndexMap.find(v)->second;
}

/////////////////////////////////////////////////////////////////////////////
void Session::requestOutput()
{
    setDetectOutput(true);
}

/////////////////////////////////////////////////////////////////////////////
void Session::expired()
{
    // Read and process the incoming data
    rxPdo();

    if (!quiet and dataTag.hasChildren())
        outbuf << dataTag << std::flush;

    dataTag.releaseChildren();

    setTimer(100);      // Wakeup in 100ms again
}

/////////////////////////////////////////////////////////////////////////////
void Session::pending()
{
//    cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    int n, count;
    size_t inputLen = 0;

    do {
        count = inbuf.free();
        n = receive(inbuf.bufptr(), count);

//        cout << std::string(inbuf.bufptr(), n);

        // No more data available or there was an error
        if (n <= 0)
            break;

        inBytes += n;   // HRTLab::Session input byte counter
        inputLen += n;  // Local input counter

        inbuf.parse(n);

    } while (n == count);

    if (!inputLen) {
        // End of stream
        delete this;
        return;
    }

    //cout << __LINE__ << " Data left in buffer: "
        //<< std::string(inbuf.bptr(), inbuf.eptr() - inbuf.bptr()) << endl;
}

/////////////////////////////////////////////////////////////////////////////
void Session::output()
{
    //cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    ssize_t n = send(outbuf.bufptr(), outbuf.size());

    // In case of error, exit
    if (n <= 0) {
        delete this;
        return;
    }

    outBytes += n;       // HRTLab::Session output byte counter

    if (outbuf.clear(n))
        setDetectOutput(false);
}

/////////////////////////////////////////////////////////////////////////////
void Session::disconnect()
{
//    cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    delete this;
}

/////////////////////////////////////////////////////////////////////////////
void Session::newSignalList(unsigned int tid,
        const HRTLab::Signal * const *v, size_t n)
{
    if (task[tid]->newSignalList(v, n)) {
        for (size_t i = 0; i < main->nst; i++) {
            task[i]->sync();
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::newSignalData(const HRTLab::Receiver &receiver)
{
    dataTag.setAttribute("time", *receiver.time);

    task[receiver.tid]->newSignalValues(&dataTag, receiver);
}

/////////////////////////////////////////////////////////////////////////////
void Session::processCommand(const char *command, const Attr &attr)
{
    size_t commandLen = strlen(command);

    static struct {
        size_t len;
        const char *name;
        void (Session::*func)(const Attr &attr);
    } cmds[] = {
        // First list most common commands
        { 4, "ping",                    &Session::ping                  },
        { 2, "rs",                      &Session::readStatistics        },
        { 2, "wp",                      &Session::writeParameter        },
        { 2, "rp",                      &Session::readParameter         },
        { 4, "xsad",                    &Session::xsad                  },
        { 4, "xsod",                    &Session::xsod                  },
        { 4, "echo",                    &Session::echo                  },

        // Now comes the rest
        { 2, "rc",                      &Session::readChannel           },
        { 2, "rk",                      &Session::readChannel           },
        { 3, "rpv",                     &Session::readParamValues       },
        { 9, "broadcast",               &Session::broadcast             },
        {11, "remote_host",             &Session::remoteHost            },
        {12, "read_kanaele",            &Session::readChannel           },
        {12, "read_statics",            &Session::readStatistics        },
        {14, "read_parameter",          &Session::readParameter },
        {15, "read_statistics",         &Session::readStatistics        },
        {15, "write_parameter",         &Session::writeParameter        },
        {17, "read_param_values",       &Session::readParamValues       },
        {0,},
    };

    // Go through the command list 
    for (size_t idx = 0; cmds[idx].len; idx++) {
        // Check whether the lengths fit and the string matches
        if (commandLen == cmds[idx].len
                and !strcmp(cmds[idx].name, command)) {

            // Call the method
            (this->*cmds[idx].func)(attr);

            // If "ack" attribute was set, send it back
            if (attr.id) {
                MsrXml::Element ack("ack");
                ack.setAttributeCheck("id", *attr.id);
                outbuf << ack << std::flush;
            }

            // Finished
            return;
        }
    }

    // Unknown command warning
    MsrXml::Element warn("warn");
    warn.setAttribute("num", 1000);
    warn.setAttribute("text", "unknown command");
    warn.setAttributeCheck("command", command);
    outbuf << warn << std::flush;
}

/////////////////////////////////////////////////////////////////////////////
void Session::broadcast(const Attr &attr)
{
    MsrXml::Element broadcast("broadcast");
    struct timespec ts;
    std::string s;

    main->gettime(&ts);

    broadcast.setAttribute("time", ts);

    if (attr.getString("action", s)) {
        broadcast.setAttributeCheck("action", s);
    }

    if (attr.getString("text",s)) {
        broadcast.setAttributeCheck("text", s);
    }
    server->broadcast(this, broadcast);
}

/////////////////////////////////////////////////////////////////////////////
void Session::echo(const Attr &attr)
{
    echoOn = attr.isTrue("value");
}

/////////////////////////////////////////////////////////////////////////////
void Session::ping(const Attr &attr)
{
    MsrXml::Element ping("ping");

    if (attr.id)
        ping.setAttributeCheck("id", *attr.id);

    outbuf << ping << std::flush;
}

/////////////////////////////////////////////////////////////////////////////
void Session::readChannel(const Attr &attr)
{
    const HRTLab::Signal *s = 0;
    bool shortReply = attr.isTrue("short");
    std::string path;
    unsigned int index;

    if (attr.getString("name", path)) {
        s = main->getSignal(path);
    }
    else if (attr.getUnsigned("index", index)) {
        if (index < signalCount)
            s = Session::signal[index];
    }
    else {
        size_t buflen = 0;
        std::map<const HRTLab::Signal*, size_t> bufOffset;
        const HRTLab::Signal *signalList[signalCount];

        typedef std::list<const HRTLab::Signal*> SignalList;
        SignalList orderedSignals[HRTLab::Variable::maxWidth];

        for (size_t i = 0; i < signalCount; i++) {
            s = signal[i];
            orderedSignals[s->width - 1].push_back(s);
        }

        index = 0;
        for (size_t w = 8; w; w /= 2) {
            for (SignalList::const_iterator it = orderedSignals[w-1].begin();
                    it != orderedSignals[w-1].end(); it++) {
                s = *it;

                signalList[index] = s;
                bufOffset[s] = buflen;
                buflen += s->memSize;

                index++;
            }
        }

        char buf[buflen];
        main->getValues(signalList, signalCount, buf);

        MsrXml::Element channels("channels");
        for (size_t i = 0; i < signalCount; i++) {
            s = signal[i];

            MsrXml::Element *c = channels.createChild("channel");
            c->setChannelAttributes(s, i, shortReply, buf + bufOffset[s]);
        }

        outbuf << channels << std::flush;
        return;
    }

    // A single signal was requested
    if (s) {
        char buf[s->memSize];
        s->getValue(buf);

        MsrXml::Element channel("channel");
        channel.setChannelAttributes(
                s, variableIndexMap[signal], shortReply, buf);

        outbuf << channel << std::flush;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::readParameter(const Attr &attr)
{
    const HRTLab::Parameter *p = 0;
    bool shortReply = attr.isTrue("short");
    bool hex = attr.isTrue("hex");
    std::string name;
    unsigned int index;
    unsigned int flags = writeAccess
        ? MSR_R | MSR_W | MSR_WOP | MSR_MONOTONIC : MSR_R | MSR_MONOTONIC;

    if (attr.getString("name", name)) {
        p = main->getParameter(name);
    }
    else if (attr.getUnsigned("index", index)) {
        if (index < parameterCount)
            p = parameter[index];
    }
    else {
        MsrXml::Element parameters("parameters");
        for (size_t i = 0; i < parameterCount; i++)
            parameters.createChild("parameter")->setParameterAttributes(
                    parameter[i], i, flags, shortReply, hex);

        outbuf << parameters << std::flush;
        return;
    }
    
    if (p) {
        MsrXml::Element parameter("parameter");
        parameter.setParameterAttributes(
                p, variableIndexMap[p], flags, shortReply, hex);
        outbuf << parameter << std::flush;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::readParamValues(const Attr &attr)
{
    MsrXml::Element param_values("param_values");
    std::string v;

    for (size_t idx = 0; idx < parameterCount; idx++) {
        if (idx) v.append(1,'|');
        char buf[parameter[idx]->memSize];
        parameter[idx]->getValue(buf);
        v.append(MsrXml::toCSV(parameter[idx], buf));
    }

    param_values.setAttribute("value", v);

    outbuf << param_values << std::flush;
}

/////////////////////////////////////////////////////////////////////////////
void Session::readStatistics(const Attr &attr)
{
    // <clients>
    //   <client index="0" name="lansim"
    //           apname="Persistent Manager, Version: 0.3.1"
    //           countin="19908501" countout="27337577"
    //           connectedtime="1282151176.659208"/>
    //   <client index="1" .../>
    // </clients>
    typedef std::list<HRTLab::SessionStatistics> StatList;
    StatList stats;
    main->getSessionStatistics(stats);
    int index = 0;

    MsrXml::Element clients("clients");
    for (StatList::const_iterator it = stats.begin();
            it != stats.end(); it++) {
        MsrXml::Element *e = clients.createChild("client");
        e->setAttribute("index", index++);
        e->setAttributeCheck("name", (*it).remote);
        e->setAttributeCheck("apname", (*it).client);
        e->setAttribute("countin", (*it).countIn);
        e->setAttribute("countout", (*it).countOut);
        e->setAttribute("connectedtime", (*it).connectedTime);
    }

    outbuf << clients << std::flush;
}

/////////////////////////////////////////////////////////////////////////////
void Session::remoteHost(const Attr &attr)
{
    attr.getString("name", HRTLab::Session::remoteHost);

    attr.getString("applicationname", client);

    writeAccess = attr.isEqual("access", "allow");

    if (writeAccess and attr.isTrue("isadmin")) {
        struct timespec ts;
        std::ostringstream os;
        main->gettime(&ts);

        os << "Adminmode filp: " << so; // 'so' is the fd and comes from
                                        // ost::Socket
        MsrXml::Element info("info");
        info.setAttribute("time", ts);
        info.setAttribute("text", os.str());
        server->broadcast(this, info);
    }
}

/////////////////////////////////////////////////////////////////////////////
class Convert {
    public:
        Convert(si_datatype_t t, char *buf): buf(buf) {
            switch (t) {
                case si_boolean_T: conv = setTo<bool>;     break;
                case si_uint8_T:   conv = setTo<uint8_t>;  break;
                case si_sint8_T:   conv = setTo<int8_t>;   break;
                case si_uint16_T:  conv = setTo<uint16_t>; break;
                case si_sint16_T:  conv = setTo<int16_t>;  break;
                case si_uint32_T:  conv = setTo<uint32_t>; break;
                case si_sint32_T:  conv = setTo<int32_t>;  break;
                case si_uint64_T:  conv = setTo<uint64_t>; break;
                case si_sint64_T:  conv = setTo<int64_t>;  break;
                case si_single_T:  conv = setTo<float>;    break;
                case si_double_T:  conv = setTo<double>;   break;
                default:           conv = 0;               break;
            }
        }
        void set(size_t idx, double val) {
            (*conv)(buf, idx, val);
        }

    private:
        char * const buf;
        void (*conv)(char *, size_t, double);

        template<class T>
        static void setTo(char *dst, size_t idx, double src) {
            reinterpret_cast<T*>(dst)[idx] = src;
        }
};

/////////////////////////////////////////////////////////////////////////////
void Session::writeParameter(const Attr &attr)
{
    if (!writeAccess) {
        MsrXml::Element warn("warn");
        warn.setAttribute("text", "No write access");
        outbuf << warn << std::flush;
        return;
    }

    const HRTLab::Parameter *p = 0;

    unsigned int index;
    std::string name;
    if (attr.getString("name", name)) {
        p = main->getParameter(name);
    }
    else if (attr.getUnsigned("index", index)) {
        if (index < parameterCount)
            p = parameter[index];
    }

    if (!p)
        return;
    
    size_t startindex = 0;
    if (attr.getUnsigned("startindex", startindex)) {
        if (startindex >= p->nelem)
            return;
    }

    char valbuf[p->memSize];
    p->getValue(valbuf);

    char *s;
    if (attr.find("hexvalue", s)) {
        static const char hexNum[] = {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,
            0,10,11,12,13,14,15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0,10,11,12,13,14,15
        };

        for (char *c = valbuf + p->width * startindex;
                c < valbuf + p->memSize; c++) {
            unsigned char c1 = *s++ - '0';
            unsigned char c2 = *s++ - '0';
            if (std::max(c1,c2) >= sizeof(hexNum))
                return;
            *c = hexNum[c1] << 4 | hexNum[c2];
        }
        // FIXME: actually the setting operation must also check for
        // endianness!
    }
    else if (attr.find("value", s)) {
        double v;
        Convert converter(p->dtype,
                valbuf + startindex * p->width);
        char c;
        std::istringstream is(s);

        is.imbue(std::locale::classic());

        for (size_t i = 0; i < p->nelem - startindex; i++) {
            is >> v;

            if (!is)
                break;

            converter.set(i, v);

            is >> c;
        }
    }
    else
        return;

    if (p->setValue(valbuf)) {
        // If an error occurred, tell this client to reread the value

        MsrXml::Element pu("pu");
        pu.setAttribute("index", variableIndexMap[p]);

        outbuf << pu << std::flush;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::xsad(const Attr &attr)
{
    std::list<const HRTLab::Signal*> channelList;
    unsigned int reduction, blocksize, precision = 10;
    bool base64 = attr.isEqual("coding", "Base64");
    bool event = attr.isTrue("event");
    bool sync = attr.isTrue("sync");
    bool foundReduction = false, foundBlocksize = false;
    std::list<unsigned int> indexList;

    // Quiet will stop all transmission of <data> tags until
    // sync is called
    quiet = !sync and attr.isTrue("quiet");

    if (attr.getUnsignedList("channels", indexList)) {
        for ( std::list<unsigned int>::const_iterator it(indexList.begin());
                it != indexList.end(); it++) {

            if (*it < signalCount)
                channelList.push_back(signal[*it]);
        }
    }
    else {
        if (sync) {
            for (size_t i = 0; i < main->nst; i++) {
                task[i]->sync();
            }
        }
        return;
    }

    if (attr.getUnsigned("reduction", reduction)) {
        if (!reduction) {
            MsrXml::Element warn("warn");
            warn.setAttribute("command", "xsad");
            warn.setAttribute("text",
                    "specified reduction=0, choosing reduction=1");
            outbuf << warn << std::flush;

            reduction = 1;
        }
        foundReduction = true;
    }

    if (attr.getUnsigned("blocksize", blocksize)) {
        if (!blocksize) {
            MsrXml::Element warn("warn");
            warn.setAttribute("command", "xsad");
            warn.setAttribute("text",
                    "specified blocksize=0, choosing blocksize=1");
            outbuf << warn << std::flush;

            blocksize = 1;
        }

        foundBlocksize = true;
    }

    if (!attr.getUnsigned("precision", precision)) {
        precision = 10;
    }


    const HRTLab::Signal *signals[channelList.size()];
    std::copy(channelList.begin(), channelList.end(), signals);
    for (const HRTLab::Signal **sp = signals;
            sp != signals + channelList.size(); sp++) {

        if (event) {
            if (!foundReduction)
                // If user did not supply a reduction, limit to a 
                // max of 10Hz automatically
                reduction = 0.1 / (*sp)->task->sampleTime
                    / (*sp)->decimation + 0.5;
        }
        else if (!foundReduction and !foundBlocksize) {
            // Quite possibly user input; choose reduction for 1Hz
            reduction = 1.0 / (*sp)->task->sampleTime
                / (*sp)->decimation + 0.5;
            blocksize = 1;
        }
        else if (foundReduction) {
            // Choose blocksize so that a datum is sent at 10Hz
            blocksize = 0.1 / (*sp)->task->sampleTime
                    / (*sp)->decimation + 0.5;
        }
        else if (foundBlocksize) {
            reduction = 1;
        }

        if (!reduction) reduction = 1;
        if (!blocksize) blocksize = 1;

        task[(*sp)->task->tid]->addSignal( *sp, getVariableIndex(*sp),
                event, sync, reduction, blocksize, base64, precision);
    }

    main->subscribe(this, signals, channelList.size());
}

/////////////////////////////////////////////////////////////////////////////
void Session::xsod(const Attr &attr)
{
    std::list<const HRTLab::Signal*> channelList;
    std::list<unsigned int> intList;

//    cout << __LINE__ << "xsod: " << endl;

    if (attr.getUnsignedList("channels", intList)) {
        for (std::list<unsigned int>::const_iterator it = intList.begin();
                it != intList.end(); it++) {
            if (*it < signalCount)
                channelList.push_back(signal[*it]);
        }

        const HRTLab::Signal *signals[channelList.size()];
        std::copy(channelList.begin(), channelList.end(), signals);
        for (Task **t = task; t != task + main->nst; t++)
            (*t)->rmSignal(signals, channelList.size());

        main->unsubscribe(this, signals, channelList.size());
    }
    else {
        for (Task **t = task; t != task + main->nst; t++)
            (*t)->rmSignal();
        main->unsubscribe(this);
    }
}
