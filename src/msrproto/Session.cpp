/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "../Main.h"
#include "../Variable.h"
#include "../Parameter.h"
#include "../Signal.h"

#include "Session.h"
#include "Server.h"
#include "Task.h"
#include "XmlDoc.h"

#include <iomanip>
#include <limits>
#include <algorithm>

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Session::Session( Server *s, ost::SocketService *ss,
        ost::TCPSocket &socket, HRTLab::Main *main):
    SocketPort(0, socket), HRTLab::Session(main),
    server(s), outbuf(this), task(main->nst),
    signal_ptr_start(main->getSignalPtrStart())
{
    cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    attach(ss);

    bptr = 0;
    writeAccess = false;
    echoOn = false;
    quiet = false;
    dataIn = 0;
    dataOut = 0;
    main->gettime(&loginTime);

    for (size_t i = 0; i < main->nst; i++) {
        task[i] = new Task(this, main);
    }

    setCompletion(false);

    MsrXml::Element greeting("connected");

    greeting.setAttribute("name", "MSR");
    greeting.setAttribute("host", "localhost");
    greeting.setAttribute("version", MSR_VERSION);
    greeting.setAttribute("features", MSR_FEATURES);
    greeting.setAttribute("recievebufsize", 100000000);

    outbuf << greeting << std::flush;

    // Skip all data in the inbox thus far
    signal_ptr = signal_ptr_start;
    while (*signal_ptr) {
        if (*signal_ptr == HRTLab::Main::Restart) {
            signal_ptr = signal_ptr_start;
            continue;
        }

        signal_ptr += signal_ptr[1];
    }

    expired();
}

/////////////////////////////////////////////////////////////////////////////
Session::~Session()
{
    cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    server->sessionClosed(this);
    for (size_t i = 0; i < main->nst; i++) {
        delete task[i];
    }
    cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
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
    //cout << __func__ << endl;
    while (*signal_ptr) {
        if (*signal_ptr == HRTLab::Main::Restart) {
            signal_ptr = signal_ptr_start;
            continue;
        }

        const size_t blockLen = signal_ptr[1];

        switch (*signal_ptr) {
            case HRTLab::Main::SubscriptionList:
                {
                    size_t headerLen = 4;
                    unsigned int tid = signal_ptr[2];
                    //unsigned int decimation = signal_ptr[3];
 
                    task[tid]->newList(signal_ptr + 4, blockLen - headerLen);
                }
                break;

            case HRTLab::Main::SubscriptionData:
                {
                    unsigned int tid = signal_ptr[2];
                    //unsigned int iterationNo = signal_ptr[3];
                    char *dataPtr = reinterpret_cast<char*>(&signal_ptr[4]);

                    struct timespec time =
                        *reinterpret_cast<const struct timespec *>(dataPtr);
                    dataPtr += sizeof(struct timespec);

//                    cout << "Main::SubscriptionData " << tid << endl;

                    MsrXml::Element data("data");
                    data.setAttribute("level", 0);
                    data.setAttribute("time", time);

                    task[tid]->newValues(&data, dataPtr);

                    if (data.hasChildren())
                        outbuf << data << std::flush;

                    data.releaseChildren();
                }
                break;
        }

        signal_ptr += blockLen;
    }

    setTimer(100);
}

/////////////////////////////////////////////////////////////////////////////
void Session::pending()
{
    //cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    int n, count;
    size_t inputLen = 0;

    do {
        char *p = inbuf.rptr();
        count = inbuf.free();
        n = receive(p, count);

        // No more data available or there was an error
        if (n <= 0)
            break;

        inbuf.rptr(n);

        dataIn += n;    // Global input counter
        inputLen += n;  // Local input counter

        char *buf = inbuf.bptr();
        parseInput(buf, inbuf.eptr());
        inbuf.erase(buf);

        //cout << __LINE__ << "n == count: " << n << ' ' << count << endl;
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
    cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
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
void Session::parseInput(char * &buf, const char * const eptr)
{
    //cout << __LINE__ << __PRETTY_FUNCTION__ << ' ' << (void*)buf << endl;
    //cout << "   ->" << std::string(buf, eptr - buf) << "<-" << endl;

    if (bptr != buf) {
        parseState = FindStart;
        bptr = buf;
        pptr = buf;
    }

    while (true) {
        switch (parseState) {
            case FindStart:
                //cout << __LINE__ << "FindStart" << endl;

                attr.clear();
                
                // Move forward in the buffer until '<' is found
                buf = std::find(buf, const_cast<char*>(eptr), '<');
                if (buf == eptr) {
                    return;
                }

                pptr = buf + 1;
                commandPtr = pptr;
                parseState = GetCommand;
                //cout << "Found command at " << (void*)commandPtr << endl;
                // no break here

            case GetCommand:
                //cout << __LINE__ << "GetCommand" << endl;
                while (pptr != eptr and (isalpha(*pptr) or *pptr == '_'))
                    pptr++;

                if (pptr == eptr)
                    return;

                commandLen = pptr - commandPtr;
                parseState = GetToken;
                cout << "Command is " << std::string(commandPtr, commandLen) << endl;
                // no break here

            case GetToken:
                //cout << __LINE__ << "GetToken" << endl;
                while (pptr != eptr and isspace(*pptr))
                    pptr++;

                if (pptr == eptr)
                    return;

                if (!isalpha(*pptr)) {
                    if (*pptr == '>') {
                        buf = pptr + 1;
                        //cout << __LINE__ << "processCommand()" << endl;
                        processCommand();
                    }
                    else if (*pptr == '/') {
                        if (eptr - pptr < 2)
                            return;

                        if (pptr[1] == '>') {
                            buf = pptr + 2;
                            //cout << __LINE__ << "processCommand()" << endl;
                            processCommand();
                        }
                        else {
                            buf = pptr + 1;
                        }
                    }
                    else {
                        buf = pptr;
                    }
                    parseState = FindStart;
                    break;
                }

                attrName = pptr;
                parseState = GetAttribute;

            case GetAttribute:
                //cout << __LINE__ << "GetAttribute" << endl;
                while (pptr != eptr and isalpha(*pptr))
                    pptr++;

                if (pptr == eptr)
                    return;

                if (*pptr == '=') {
                    // Value attribute
                    attrLen = pptr - attrName;
                    pptr++;
                }
                else {
                    if (isspace(*pptr)) {
                        // Binary attribute
                        attr.insert(attrName, pptr - attrName);
                    }
                    parseState = GetToken;
                    break;
                }

                parseState = GetQuote;
                // no break here

            case GetQuote:
                //cout << __LINE__ << "GetQuote" << endl;
                if (pptr == eptr)
                    return;

                quote = *pptr;
                if (quote != '\'' and quote != '"') {
                    buf = pptr;
                    parseState = FindStart;
                    break;
                }

                pptr++;
                attrValue = pptr;
                parseState = GetValue;
                // no break here

            case GetValue:
                //cout << __LINE__ << "GetValue" << endl;
                pptr = std::find(pptr, const_cast<char*>(eptr), quote);

                if (pptr == eptr)
                    return;

                attr.insert(attrName, attrLen,
                        attrValue, pptr - attrValue);
                pptr++;
                
                parseState = GetToken;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::processCommand()
{
    static struct {
        size_t len;
        const char *name;
        void (Session::*func)();
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
                and !strncmp(cmds[idx].name, commandPtr, commandLen)) {
            pptr += cmds[idx].len;
            (this->*cmds[idx].func)();
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
    warn.setAttributeCheck("command", commandPtr, commandLen);
    outbuf << warn << std::flush;
}

/////////////////////////////////////////////////////////////////////////////
void Session::broadcast()
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
void Session::echo()
{
    echoOn = attr.isTrue("value");
}

/////////////////////////////////////////////////////////////////////////////
void Session::ping()
{
    MsrXml::Element ping("ping");

    if (attr.id)
        ping.setAttributeCheck("id", *attr.id);

    outbuf << ping << std::flush;
}

/////////////////////////////////////////////////////////////////////////////
void Session::readChannel()
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
        double freq = main->baserate / signal->decimation
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
            double freq = main->baserate / signal[i]->decimation
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
void Session::readParameter()
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
void Session::readParamValues()
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
void Session::readStatistics()
{
    // <clients>
    //   <client index="0" name="lansim"
    //           apname="Persistent Manager, Version: 0.3.1"
    //           countin="19908501" countout="27337577"
    //           connectedtime="1282151176.659208"/>
    //   <client index="1" .../>
    // </clients>
    const std::list<const HRTLab::Session*> s(main->getSessions());
    int index = 0;

    MsrXml::Element clients("clients");
    for (std::list<const HRTLab::Session*>::const_iterator it = s.begin();
            it != s.end(); it++) {
        MsrXml::Element *e = clients.createChild("client");
        e->setAttribute("index", index++);
        e->setAttributeCheck("name", (*it)->getName());
        e->setAttributeCheck("apname", (*it)->getClientName());
        e->setAttribute("countin", (*it)->getCountIn());
        e->setAttribute("countout", (*it)->getCountOut());
        e->setAttribute("connectedtime", (*it)->getLoginTime());
    }

    outbuf << clients << std::flush;
}

/////////////////////////////////////////////////////////////////////////////
void Session::remoteHost()
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
void Session::writeParameter()
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
void Session::xsad()
{
    std::list<HRTLab::Signal*> channelList;
    unsigned int reduction, blocksize, precision = 10;
    bool base64 = attr.isEqual("coding", "Base64");
    const HRTLab::Main::SignalList& sl = main->getSignals();
    std::list<unsigned int> indexList;

    cout << __LINE__ << "xsad: " << std::string(bptr, pptr - bptr) << endl;

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

    if (!attr.getUnsigned("reduction", reduction)) {
        reduction = 1 / main->baserate;
    }

    if (!attr.getUnsigned("blocksize", blocksize)) {
        blocksize = main->baserate;
    }

    if (!attr.getUnsigned("precision", precision)) {
        precision = 10;
    }

    if (attr.isTrue("quiet")) {
        for (std::vector<Task*>::iterator it = task.begin();
                it != task.end(); it++) {
            (*it)->setQuiet(true);
        }
    }

    if (attr.isTrue("sync")) {
        for (std::vector<Task*>::iterator it = task.begin();
                it != task.end(); it++) {
            (*it)->setQuiet(false);
        }
    }

    const HRTLab::Signal *signals[channelList.size()];
    std::copy(channelList.begin(), channelList.end(), signals);
    for (const HRTLab::Signal **sp = signals;
            sp != signals + channelList.size(); sp++) {
        task[(*sp)->tid]->addSignal(*sp, reduction, blocksize, base64, precision);
    }

    main->subscribe(this, signals, channelList.size());
}

/////////////////////////////////////////////////////////////////////////////
void Session::xsod()
{
    std::list<HRTLab::Signal*> channelList;
    std::list<unsigned int> intList;
    const HRTLab::Main::SignalList& sl = main->getSignals();

    cout << __LINE__ << "xsod: " << std::string(bptr, pptr - bptr) << endl;

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
        main->clearSession(this);
    }

}
