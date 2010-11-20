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
#include "XmlDoc.h"

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
    server(s), outbuf(this), inbuf(this), task(main->nst)
{
//    cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    attach(ss);

    writeAccess = false;
    echoOn = false;
    quiet = false;
    dataIn = 0;
    dataOut = 0;
    main->gettime(&loginTime);

    for (size_t i = 0; i < main->nst; i++) {
        task[i] = new Task();
    }

    setCompletion(false);

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
void Session::parameterChanged(const HRTLab::Parameter *p)
{
    MsrXml::Element pu("pu");
    pu.setAttribute("index", p->index);

    outbuf << pu << std::flush;
}

/////////////////////////////////////////////////////////////////////////////
void Session::requestOutput()
{
    setDetectOutput(true);
}

/////////////////////////////////////////////////////////////////////////////
void Session::expired()
{
    main->rxPdo(this);
    setTimer(100);
//
//    if (rand > 0) {
//        int channel = rand() % 2;
//        main->
//        for (const HRTLab::Signal **sp = signals;
//                sp != signals + channelList.size(); sp++) {
//            cout << "xsad " << (*sp)->index << (*sp)->path << endl;
//            task[(*sp)->tid]->addSignal(*sp, reduction, blocksize, base64, precision);
//        }
//    }
//    else {
//    }
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

        cout << std::string(inbuf.bufptr(), n) << endl;
        // No more data available or there was an error
        if (n <= 0)
            break;

        dataIn += n;    // Global input counter
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

    dataOut += n;

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
std::string Session::getName() const
{
    return remote;
}

/////////////////////////////////////////////////////////////////////////////
std::string Session::getClientName() const
{
    return applicationname;
}

/////////////////////////////////////////////////////////////////////////////
size_t Session::getCountIn() const
{
    return dataIn;
}

/////////////////////////////////////////////////////////////////////////////
size_t Session::getCountOut() const
{
    return dataOut;
}

/////////////////////////////////////////////////////////////////////////////
struct timespec Session::getLoginTime() const
{
    return loginTime;
}

/////////////////////////////////////////////////////////////////////////////
void Session::newVariableList(const HRTLab::Task *t,
        const HRTLab::Variable **s, size_t n)
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

    MsrXml::Element data("data");
    data.setAttribute("level", 0);
    data.setAttribute("time", *time);

    task[t->tid]->newValues(&data, seqNo, dataPtr);

    if (data.hasChildren())
        outbuf << data << std::flush;

    data.releaseChildren();
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
        { 2, "rs",                      &Session::readStatistics},
        { 2, "wp",                      &Session::writeParameter,},
        { 2, "rp",                      &Session::readParameter,},
        { 4, "ping",                    &Session::ping,},
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

    for (size_t idx = 0; cmds[idx].len; idx++) {
        if (commandLen == cmds[idx].len
                and !strcmp(cmds[idx].name, command)) {
            (this->*cmds[idx].func)(attr);
            if (attr.id) {
                MsrXml::Element ack("ack");
                ack.setAttributeCheck("id", *attr.id);
                outbuf << ack << std::flush;
            }
            return;
        }
    }

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

    server->getTime(ts);

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
            return;

        signal = sl[vit->second->index];
    }
    else if (attr.getUnsigned("index", index)) {
        if (index >= sl.size())
            return;

        signal = sl[index];
    }


    if (signal) {
        char buf[signal->memSize];
        double freq = 1.0 / main->baserate / signal->decimation
                / (main->decimation ? main->decimation[signal->tid] : 1);
        size_t bufsize = std::max( 1U, (size_t)freq);

        main->poll(&signal, 1, buf);

        MsrXml::Element channel("channel");
        channel.setChannelAttributes(signal, shortReply, freq, bufsize);
        channel.setAttribute("value", MsrXml::toCSV(signal, buf));

        outbuf << channel;
    }
    else {
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

            size_t bufsize = std::max( 1U, (size_t)freq);

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
            return;

        parameter = pl[vit->second->index];
    }
    else if (attr.getUnsigned("index", index)) {
        if (index >= pl.size())
            return;

        parameter = pl[index];
    }
    
    if (parameter) {
        MsrXml::Element p("parameter");
        p.setParameterAttributes(parameter, shortReply, hex);
        outbuf << p << std::flush;
    }
    else {
        MsrXml::Element parameters("parameters");
        for (HRTLab::Main::ParameterList::const_iterator it = pl.begin();
                it != pl.end(); it++) {
            MsrXml::Element *p = parameters.createChild("parameter");
            p->setParameterAttributes(*it, shortReply, hex);
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
        v.append(MsrXml::toCSV(*it, (*it)->Variable::addr));
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
//    const HRTLab::Main::SessionSet& s(main->getSessions());
//    int index = 0;
//
//    MsrXml::Element clients("clients");
//    for (HRTLab::Main::SessionSet::const_iterator it = s.begin();
//            it != s.end(); it++) {
//        MsrXml::Element *e = clients.createChild("client");
//        e->setAttribute("index", index++);
//        e->setAttributeCheck("name", (*it)->getName());
//        e->setAttributeCheck("apname", (*it)->getClientName());
//        e->setAttribute("countin", (*it)->getCountIn());
//        e->setAttribute("countout", (*it)->getCountOut());
//        e->setAttribute("connectedtime", (*it)->getLoginTime());
//    }
//
//    outbuf << clients << std::flush;
}

/////////////////////////////////////////////////////////////////////////////
void Session::remoteHost(const Attr &attr)
{
    attr.getString("name", remote);

    attr.getString("applicationname", applicationname);

    writeAccess = attr.isEqual("access", "allow");

    if (writeAccess and attr.isTrue("isadmin")) {
        struct timespec ts;
        std::ostringstream os;
        server->getTime(ts);

        os << "Adminmode filp: " << so; // so is the fd and comes from
        // ost::Socket
        MsrXml::Element info("info");
        info.setAttribute("time", ts);
        info.setAttributeCheck("text", os.str());
        server->broadcast(this, info);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::writeParameter(const Attr &attr)
{
    if (!writeAccess) {
        MsrXml::Element warn("warn");
        warn.setAttribute("text", "No write access");
        outbuf << warn << std::flush;
        return;
    }

//    HRTLab::Parameter *parameter;
//    size_t startindex = 0;
//    const HRTLab::Main::ParameterList& pl = main->getParameters();

//    if ((it = attributes.find("name")) != attributes.end()) {
//        const HRTLab::Main::VariableMap& m = main->getVariableMap();
//        HRTLab::Main::VariableMap::const_iterator vit = m.find(it->second);
//
//        if (vit == m.end() or vit->second->index >= pl.size()
//                or pl[vit->second->index]->path != it->second)
//            return;
//
//        parameter = pl[vit->second->index];
//    }
//    else if ((it = attributes.find("index")) != attributes.end()) {
//        unsigned int index = atoi(it->second.c_str());
//
//        if (index >= pl.size())
//            return;
//
//        parameter = pl[index];
//    }
//    else {
//        return;
//    }
//
//    if ((it = attributes.find("startindex")) != attributes.end()) {
//        startindex = atoi(it->second.c_str());
//    }
//
//    char valbuf[parameter->memSize];
//    size_t validx = 0;
//    if ((it = attributes.find("hexvalue")) != attributes.end()) {
//        const std::string &s = it->second;
//        const char hexNum[] = {
//            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,
//            0,10,11,12,13,14,15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
//            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
//            0,10,11,12,13,14,15
//        };
//
//        for (validx = 0;
//                validx < std::min(parameter->memSize, s.size() / 2);
//                validx++) {
//            unsigned char c1 = s[validx*2] - '0';
//            unsigned char c2 = s[validx*2 + 1] - '0';
//            if (c1 > 'f' - '0' or c2 > 'f' - '0')
//                return;
//            valbuf[validx] = hexNum[c1] << 4 | hexNum[c2];
//        }
//    }
//    else if ((it = attributes.find("value")) != attributes.end()) {
//        double v;
//        char c;
//        std::istringstream is(it->second);
//
//        is.imbue(std::locale::classic());
//
//        for (validx = 0; validx < parameter->nelem; validx++) {
//            is >> v;
//            cout << "value = " << v << endl;
//
//            if (!is)
//                break;
//
//            switch (parameter->dtype) {
//                case si_boolean_T:
//                    reinterpret_cast<bool*>(valbuf)[validx] = v; break;
//
//                case si_uint8_T:
//                    reinterpret_cast<uint8_t*>(valbuf)[validx] = v; break;
//
//                case si_sint8_T:
//                    reinterpret_cast<int8_t*>(valbuf)[validx] = v; break;
//
//                case si_uint16_T:
//                    reinterpret_cast<uint16_t*>(valbuf)[validx] = v; break;
//
//                case si_sint16_T:
//                    reinterpret_cast<int16_t*>(valbuf)[validx] = v; break;
//
//                case si_uint32_T:
//                    reinterpret_cast<uint32_t*>(valbuf)[validx] = v; break;
//
//                case si_sint32_T:
//                    reinterpret_cast<int32_t*>(valbuf)[validx] = v; break;
//
//                case si_uint64_T:
//                    reinterpret_cast<uint64_t*>(valbuf)[validx] = v; break;
//
//                case si_sint64_T:
//                    reinterpret_cast<int64_t*>(valbuf)[validx] = v; break;
//
//                case si_single_T:
//                    reinterpret_cast<float*>(valbuf)[validx] = v; break;
//
//                case si_double_T:
//                    reinterpret_cast<double*>(valbuf)[validx] = v; break;
//
//                default:
//                    break;
//            }
//
//            is >> c;
//        }
//    }

//    parameter->setValue(valbuf, validx, startindex);
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

            if (*it >= sl.size()) {
                std::ostringstream os;
                os << "Channel " << *it << " does not exist.";

                MsrXml::Element warn("warn");
                warn.setAttribute("text", os.str());
                warn.setAttribute("command", "xsad");
                outbuf << warn << std::flush;

                return;
            }

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
        blocksize = 1;
        if (!foundReduction)
            reduction = std::max(1.0, 0.1 / main->baserate + 0.5);
    }
    else {
        if (!foundReduction and !foundBlocksize) {
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
    }

//    cout << "foundReduction " << event << main->baserate << " reduction " << reduction << endl;

    if (!attr.getUnsigned("precision", precision)) {
        precision = 10;
    }

    if (attr.isTrue("quiet"))
        quiet = true;

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
//        cout << "xsad " << (*sp)->index << (*sp)->path << endl;
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
