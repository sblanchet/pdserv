/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "config.h"

#include "../Main.h"
#include "../Task.h"
#include "../Variable.h"
#include "../Parameter.h"
#include "../Signal.h"

#include "Session.h"
#include "Server.h"
#include "Task.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Session::Session( Server *s, ost::SocketService *ss,
        ost::TCPSocket &socket, HRTLab::Main *main):
    SocketPort(0, socket), HRTLab::Session(main),
    server(s), dataTag("data"), outbuf(this), inbuf(this), task(main->nst)
{
//    cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    attach(ss);

    // Setup some internal variables
    writeAccess = false;
    echoOn = false;
    quiet = false;
    dataTag.setAttribute("level", 0);

    // Create enough tasks
    for (size_t i = 0; i < main->nst; i++) {
        task[i] = new Task();
    }

    // Non-blocking read() and write()
    setCompletion(false);

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
        pu.setAttribute("index", (*p++)->index);
        outbuf << pu << std::flush;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::requestOutput()
{
    setDetectOutput(true);
}

/////////////////////////////////////////////////////////////////////////////
void Session::expired()
{
    main->rxPdo(this);  // Read and process the incoming data
    setTimer(100);      // Wakeup in 100ms again
}

/////////////////////////////////////////////////////////////////////////////
void Session::pending()
{
    //cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    int n, count;
    size_t inputLen = 0;

    do {
        count = inbuf.free();
        n = receive(inbuf.bufptr(), count);

        //cout << std::string(inbuf.bufptr(), n);
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
void Session::newVariableList(const HRTLab::Task *t,
        const HRTLab::Variable * const *s, size_t n)
{
//    cout << __PRETTY_FUNCTION__ << endl;
    task[t->tid]->newVariableList(s, n);
}

/////////////////////////////////////////////////////////////////////////////
void Session::newPdoData(const HRTLab::Task *t, unsigned int seqNo,
                const struct timespec *time, const char *dataPtr)
{
    if (quiet)
        return;

    dataTag.setAttribute("time", *time);

    task[t->tid]->newValues(&dataTag, seqNo, dataPtr);

    if (dataTag.hasChildren())
        outbuf << dataTag << std::flush;

    dataTag.releaseChildren();
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
        { 4, "ping",                    &Session::ping,},
        { 2, "rs",                      &Session::readStatistics},
        { 2, "wp",                      &Session::writeParameter,},
        { 2, "rp",                      &Session::readParameter,},
        { 4, "xsad",                    &Session::xsad,},
        { 4, "xsod",                    &Session::xsod,},
        { 4, "echo",                    &Session::echo,},

        // Now comes the rest
        { 2, "rc",                      &Session::readChannel,},
        { 2, "rk",                      &Session::readChannel,},
        { 3, "rpv",                     &Session::readParamValues,},
        { 9, "broadcast",               &Session::broadcast,},
        {11, "remote_host",             &Session::remoteHost,},
        {12, "read_kanaele",            &Session::readChannel,},
        {12, "read_statics",            &Session::readStatistics,},
        {14, "read_parameter",          &Session::readParameter,},
        {15, "read_statistics",         &Session::readStatistics,},
        {15, "write_parameter",         &Session::writeParameter,},
        {17, "read_param_values",       &Session::readParamValues,},
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
    HRTLab::Signal *signal = 0;
    bool shortReply = attr.isTrue("short");
    const HRTLab::Main::SignalList& sl = main->getSignals();
    std::string path;
    unsigned int index;

    if (attr.getString("name", path)) {
        const HRTLab::Main::VariableMap& m = main->getVariableMap();
        HRTLab::Main::VariableMap::const_iterator vit = m.find(path);

        if (vit == m.end() or vit->second->index >= sl.size()
                or sl[vit->second->index]->path != path)
            // Signal does not exist, ignore it
            return;

        signal = sl[vit->second->index];
    }
    else if (attr.getUnsigned("index", index)) {
        if (index >= sl.size())
            // Index is out of range, ignore it
            return;

        signal = sl[index];
    }


    if (signal) {
        // A single signal was requested

        char buf[signal->memSize];
        double freq = 1.0 / main->baserate / signal->decimation
                / (main->decimation ? main->decimation[signal->tid] : 1);
        size_t bufsize = std::max( 1U, (size_t)(freq + 0.5));

        main->poll(&signal, 1, buf);

        MsrXml::Element channel("channel");
        channel.setChannelAttributes(signal, shortReply, freq, bufsize);
        channel.setAttribute("value", MsrXml::toCSV(signal, buf));

        outbuf << channel;
    }
    else {
        // Send the whole list

        HRTLab::Signal *signal[sl.size()];
        size_t buflen = 0;

        for (size_t i = 0; i < sl.size(); i++) {
            signal[i] = sl[i];
            buflen += signal[i]->memSize;
        }
        char buf[buflen];
        main->poll(signal, sl.size(), buf);

        MsrXml::Element channels("channels");
        const char *p = buf;
        for (size_t i = 0; i < sl.size(); i++) {
            double freq = 1.0 / main->baserate / signal[i]->decimation
                    / (main->decimation ? main->decimation[signal[i]->tid] : 1);

            // The MSR protocoll wants a bufsize, the maximum number of
            // values that can be retraced. This artificial limitation does
            // not exist any more. Instead, choose a buffer size so that
            // at a maximum of 1 second has to be stored.
            size_t bufsize = std::max( 1U, (size_t)(freq + 0.5));

            MsrXml::Element *c = channels.createChild("channel");
            c->setChannelAttributes(signal[i], shortReply, freq, bufsize);
            c->setAttribute("value", MsrXml::toCSV(signal[i], p));
            p += signal[i]->memSize;
        }
        outbuf << channels;
    }

    outbuf << std::flush;
}

/////////////////////////////////////////////////////////////////////////////
void Session::readParameter(const Attr &attr)
{
    HRTLab::Parameter *parameter = 0;
    bool shortReply = attr.isTrue("short");
    bool hex = attr.isTrue("hex");
    std::string name;
    unsigned int index;
    const HRTLab::Main::ParameterList& pl = main->getParameters();

    if (attr.getString("name", name)) {
        const HRTLab::Main::VariableMap& m = main->getVariableMap();
        HRTLab::Main::VariableMap::const_iterator vit = m.find(name);

        if (vit == m.end() or vit->second->index >= pl.size()
                or pl[vit->second->index]->path != name)
            // Parameter does not exist, ignore
            return;

        parameter = pl[vit->second->index];
    }
    else if (attr.getUnsigned("index", index)) {
        if (index >= pl.size())
            // Index does not exist, ignore
            return;

        parameter = pl[index];
    }
    
    if (parameter) {
        MsrXml::Element p("parameter");
        p.setParameterAttributes(parameter, main->getParameterAddr(parameter),
                main->getMTime(parameter), shortReply, hex);
        outbuf << p << std::flush;
    }
    else {
        MsrXml::Element parameters("parameters");
        for (HRTLab::Main::ParameterList::const_iterator it = pl.begin();
                it != pl.end(); it++) {
            MsrXml::Element *p = parameters.createChild("parameter");
            p->setParameterAttributes(*it, main->getParameterAddr(*it),
                    main->getMTime(*it), shortReply, hex);
        }
        outbuf << parameters << std::flush;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::readParamValues(const Attr &attr)
{
    MsrXml::Element param_values("param_values");
    std::string v;
    bool separator = false;
    const HRTLab::Main::ParameterList& pl = main->getParameters();

    for (HRTLab::Main::ParameterList::const_iterator it = pl.begin();
            it != pl.end(); it++) {
        if (separator) v.append(1,'|');
        v.append(MsrXml::toCSV(*it, main->getParameterAddr(*it)));
        separator = true;
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
    typedef std::list<HRTLab::Session::Statistics> StatList;
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

    const HRTLab::Parameter *parameter;
    size_t startindex = 0;
    const HRTLab::Main::ParameterList& pl = main->getParameters();
    unsigned int index;
    std::string name;

    if (attr.getString("name", name)) {
        const HRTLab::Main::VariableMap& m = main->getVariableMap();
        HRTLab::Main::VariableMap::const_iterator vit = m.find(name);

        if (vit == m.end() or vit->second->index >= pl.size()
                or pl[vit->second->index]->path != name)
            // Parameter does not exist, ignore
            return;

        parameter = pl[vit->second->index];
    }
    else if (attr.getUnsigned("index", index)) {
        if (index >= pl.size())
            // Index does not exist, ignore
            return;

        parameter = pl[index];
    }
    else
        return;
    
    if (attr.getUnsigned("startindex", startindex)) {
        if (startindex >= parameter->nelem)
            return;
    }

    char valbuf[parameter->memSize];
    char *s;
    std::copy(main->getParameterAddr(parameter),
            main->getParameterAddr(parameter) + parameter->memSize, valbuf);
    if (attr.find("hexvalue", s)) {
        static const char hexNum[] = {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,
            0,10,11,12,13,14,15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0,10,11,12,13,14,15
        };

        for (char *c = valbuf + parameter->width * startindex;
                c < valbuf + parameter->memSize; c++) {
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
        Convert converter(parameter->dtype,
                valbuf + startindex * parameter->width);
        char c;
        std::istringstream is(s);

        is.imbue(std::locale::classic());

        for (size_t i = 0; i < parameter->nelem - startindex; i++) {
            is >> v;

            if (!is)
                break;

            converter.set(i, v);

            is >> c;
        }
    }
    else
        return;

    if (main->writeParameter(&parameter, 1, valbuf)) {
        // If an error occurred, tell this client to reread the value

        MsrXml::Element pu("pu");
        pu.setAttribute("index", parameter->index);

        outbuf << pu << std::flush;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::xsad(const Attr &attr)
{
    std::list<HRTLab::Signal*> channelList;
    unsigned int reduction, blocksize, precision = 10;
    bool base64 = attr.isEqual("coding", "Base64");
    bool event = attr.isTrue("event");
    bool foundReduction = false, foundBlocksize = false;
    const HRTLab::Main::SignalList& sl = main->getSignals();
    std::list<unsigned int> indexList;

    if (attr.getUnsignedList("channels", indexList)) {
        for ( std::list<unsigned int>::const_iterator it(indexList.begin());
                it != indexList.end(); it++) {

            if (*it <= sl.size())
                channelList.push_back(sl[*it]);
        }
    }
    else
        return;

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

    if (event) {
        if (!foundReduction)
            // If user did not supply a reduction, limit to a 
            // max of 10Hz automatically
            reduction = std::max(1.0, 0.1 / main->baserate + 0.5);
    }
    else if (!foundReduction and !foundBlocksize) {
        // Quite possibly user input; choose reduction for 1Hz
        reduction = std::max(1.0, 1.0 / main->baserate + 0.5);
        blocksize = 1;
    }
    else if (foundReduction) {
        // Choose blocksize so that a datum is sent at 10Hz
        blocksize =
            std::max(1.0, 0.1 / main->baserate / reduction + 0.5);
    }
    else if (foundBlocksize) {
        reduction = 1;
    }

    if (!attr.getUnsigned("precision", precision)) {
        precision = 10;
    }

    // Quiet will stop all transmission of <data> tags until
    // sync is called
    quiet = attr.isTrue("quiet");

    // Calling sync will reset all the channel buffers so that they
    // all start together again
    if (attr.isTrue("sync")) {
        for (std::vector<Task*>::iterator it = task.begin();
                it != task.end(); it++) {
            (*it)->sync();
        }
        quiet = false;
    }

    const HRTLab::Signal *signals[channelList.size()];
    std::copy(channelList.begin(), channelList.end(), signals);
    for (const HRTLab::Signal **sp = signals;
            sp != signals + channelList.size(); sp++) {
        task[(*sp)->tid]->addSignal(
                *sp, event, reduction, blocksize, base64, precision);
    }

    main->subscribe(this, signals, channelList.size());
}

/////////////////////////////////////////////////////////////////////////////
void Session::xsod(const Attr &attr)
{
    std::list<HRTLab::Signal*> channelList;
    std::list<unsigned int> intList;
    const HRTLab::Main::SignalList& sl = main->getSignals();

//    cout << __LINE__ << "xsod: " << endl;

    if (attr.getUnsignedList("channels", intList)) {
        for (std::list<unsigned int>::const_iterator it = intList.begin();
                it != intList.end(); it++) {
            if (*it < sl.size())
                channelList.push_back(sl[*it]);
        }

        const HRTLab::Signal *signals[channelList.size()];
        std::copy(channelList.begin(), channelList.end(), signals);
        for (const HRTLab::Signal **sp = signals;
                sp != signals + channelList.size(); sp++) {
            task[(*sp)->tid]->rmSignal(*sp);
        }

        main->unsubscribe(this, signals, channelList.size());
    }
    else {
        for (std::vector<Task*>::iterator it = task.begin();
                it != task.end(); it++) {
            (*it)->rmSignal(0);
        }
        main->unsubscribe(this);
    }
}
