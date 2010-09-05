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

Session::CommandMap Session::commandMap;

/////////////////////////////////////////////////////////////////////////////
Session::Session( Server *s, ost::SocketService *ss,
        ost::TCPSocket &socket, HRTLab::Main *main):
    SocketPort(0, socket), std::ostream(this),
    main(main), server(s), task(main->nst),
    signal_ptr_start(main->getSignalPtrStart())
{
    cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    attach(ss);

    writeAccess = false;
    echo = false;
    quiet = false;
    for (size_t i = 0; i < main->nst; i++) {
        task[i] = new Task(this);
    }
    signal_ptr = signal_ptr_start;

    if (commandMap.empty()) {
        commandMap["ping"] = &Session::pingCmd;
        commandMap["rp"] = &Session::readParameterCmd;
        commandMap["read_parameter"] = &Session::readParameterCmd;
        commandMap["broadcast"] = &Session::broadcastCmd;
        commandMap["read_param_values"] = &Session::readParamValuesCmd;
        commandMap["rpv"] = &Session::readParamValuesCmd;
        commandMap["rk"] = &Session::readChannelsCmd;
        commandMap["rc"] = &Session::readChannelsCmd;
        commandMap["read_kanaele"] = &Session::readChannelsCmd;
        commandMap["wp"] = &Session::writeParameterCmd;
        commandMap["write_parameter"] = &Session::writeParameterCmd;
        commandMap["echo"] = &Session::echoCmd;
        commandMap["remote_host"] = &Session::remoteHostCmd;

        commandMap["xsad"] = &Session::xsadCmd;
        commandMap["xsod"] = &Session::xsodCmd;
        commandMap["read_statics"] = &Session::readStatisticsCmd;
        commandMap["read_statistics"] = &Session::readStatisticsCmd;
        commandMap["rs"] = &Session::readStatisticsCmd;
        commandMap["te"] = &Session::broadcastCmd; //triggerevents

        //commandMap["start_data"] = &Session::xsadCmd;
        //commandMap["stop_data"] = &Session::xsadCmd;
        //commandMap["sad"] = &Session::xsadCmd;
        //commandMap["sod"] = &Session::xsodCmd;
    }


    setCompletion(false);

    MsrXml::Element greeting("connected");

    greeting.setAttribute("name", "MSR");
    greeting.setAttribute("host", "localhost");
    greeting.setAttribute("version", MSR_VERSION);
    greeting.setAttribute("features", MSR_FEATURES);
    greeting.setAttribute("recievebufsize", "100000000");

    *this << greeting << std::flush;

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
}

/////////////////////////////////////////////////////////////////////////////
int Session::sync()
{
    if (!buf.empty())
        setDetectOutput(true);
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
int Session::overflow(int c)
{
    buf.append(1,c);
    return c;
}

/////////////////////////////////////////////////////////////////////////////
std::streamsize Session::xsputn ( const char * s, std::streamsize n )
{
    buf.append(s,n);
    return n;
}

/////////////////////////////////////////////////////////////////////////////
void Session::newValue(const HRTLab::Signal *s, const char *dataPtr,
                struct timespec &t)
{
    *this << "Newvalue for " << s->path << endl << std::flush;
}

/////////////////////////////////////////////////////////////////////////////
void Session::expired()
{
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

                    cout << "Main::SubscriptionData " << tid << endl;

                    task[tid]->newValues(dataPtr);
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
    cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    char buf[1024];

    ssize_t n = receive(buf, sizeof(buf));
    if (n <= 0) {
        delete this;
        return;
    }

    const char *pptr, *eptr;
    if (inbuf.empty()) {
        pptr = buf;
        eptr = buf + n;
    }
    else {
        inbuf.append(buf,n);
        pptr = inbuf.c_str();
        eptr = inbuf.c_str() + inbuf.size();
    }

    while (true) {
        if (search('<', pptr, eptr))
            break;

        const char *p = pptr;

        if (evalExpression(p, eptr))
            break;

        pptr = p;
    }

    if (inbuf.empty()) {
        inbuf.append(pptr, eptr - pptr);
    }
    else {
        inbuf.erase(0, pptr - buf);
    }
}

/////////////////////////////////////////////////////////////////////////////
bool Session::search(char c, const char* &pptr, const char *eptr)
{
    pptr = std::find(pptr, eptr, c);
    return pptr == eptr;
}

/////////////////////////////////////////////////////////////////////////////
bool Session::evalExpression(const char* &pptr, const char *eptr)
{
    const char *start = pptr;

    if (*pptr++ != '<')
        return false;

    const char * const cmd = pptr;
    if (evalIdentifier(pptr, eptr))
        return true;

    const size_t cmdLen = pptr - cmd;
    AttributeMap attributes;

    std::string id;
    while (true) {
        std::string name;
        std::string value;

        if (evalAttribute(pptr, eptr, name, value))
            return true;

        if (name.empty()) {
            // No attributes any more
            break;
        }

        if (name == "id")
            id = value;

        attributes[name] = value;
    }

    // Skip whitespace
    while (isspace(*pptr)) {
        if (++pptr == eptr)
            return true;
    }

    if (*pptr == '/')
        if (++pptr == eptr)
            return true;

    if (*pptr++ != '>')
        return false;

    if (echo) {
        *this << std::string(start, pptr - start) << "\r\n" << std::flush;
    }

    CommandMap::const_iterator it = commandMap.find(std::string(cmd,cmdLen));
    if (it == commandMap.end()) {
        MsrXml::Element warn("warn");
        warn.setAttribute("num","1000");
        warn.setAttribute("text", "unknown command");
        warn.setAttribute("command", cmd, cmdLen);
        *this << warn << std::flush;

        return false;
    }

    // Execute the desired command
    (this->*(it->second))(attributes);

    // Return the id sent previously
    if (!id.empty()) {
        MsrXml::Element ack("ack");
        ack.setAttribute("id", id);
        *this << ack << std::flush;
    }

    return false;
}

/////////////////////////////////////////////////////////////////////////////
bool Session::evalAttribute(const char* &pptr, const char *eptr,
        std::string &name, std::string &value)
{
    const char *start = pptr;
    while (isspace(*pptr)) 
        if (++pptr == eptr)
            return true;

    if (pptr == start) {
        // No whitespace found
        return false;
    }

    start = pptr;
    if (evalIdentifier(pptr, eptr))
        return true;

    // At the end of the identifier, need at least 3 more chars ( ="" )
    if (eptr - pptr < 3)
        return true;

    char quote = pptr[1];
    if (pptr[0] != '=' or (quote != '"' and quote != '\''))
        return false;
    name = std::string(start, pptr - start);

    pptr += 2;
    size_t len = 0;
    while (pptr[len] != quote) {
        if (pptr + ++len == eptr)
            return true;
    }
    value = std::string(pptr, len);

    pptr += len + 1;            // Skip the quote

    return false;
}

/////////////////////////////////////////////////////////////////////////////
bool Session::evalIdentifier(const char* &pptr, const char *eptr)
{
    if (!isalpha(*pptr))
        return false;

    while (isalnum(*pptr) or *pptr == '_') {
        if (++pptr == eptr)
            return true;
    }

    return false;
}

/////////////////////////////////////////////////////////////////////////////
void Session::pingCmd(const AttributeMap &attributes)
{
    MsrXml::Element ping("ping");

    AttributeMap::const_iterator it = attributes.find("id");
    if (it != attributes.end())
        ping.setAttribute("id", it->second);

    *this << ping << std::flush;
}

/////////////////////////////////////////////////////////////////////////////
void Session::readParameterCmd(const AttributeMap &attributes)
{
    AttributeMap::const_iterator it;
    HRTLab::Parameter *parameter = 0;
    bool shortReply = false;
    bool hex = false;
    const HRTLab::Main::ParameterList& pl = main->getParameters();

    if ((it = attributes.find("short"))!= attributes.end()) {
        shortReply = atoi(it->second.c_str());
    }

    if ((it = attributes.find("hex"))!= attributes.end()) {
        hex = atoi(it->second.c_str());
    }

    if ((it = attributes.find("name")) != attributes.end()) {
        const HRTLab::Main::VariableMap& m = main->getVariableMap();
        HRTLab::Main::VariableMap::const_iterator vit = m.find(it->second);

        if (vit == m.end() or vit->second->index >= pl.size()
                or pl[vit->second->index]->path != it->second)
            return;

        parameter = pl[vit->second->index];
    }
    else if ((it = attributes.find("index")) != attributes.end()) {
        unsigned int index = atoi(it->second.c_str());

        if (index >= pl.size())
            return;

        parameter = pl[index];
    }
    
    if (parameter) {
        MsrXml::Element p("parameter");
        setParameterAttributes(&p, parameter, shortReply, hex);
        *this << p << std::flush;
    }
    else {
        MsrXml::Element parameters("parameters");
        for (HRTLab::Main::ParameterList::const_iterator it = pl.begin();
                it != pl.end(); it++) {
            MsrXml::Element *p = parameters.createChild("parameter");
            setParameterAttributes(p, *it, shortReply, hex);
        }
        *this << parameters << std::flush;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::setParameterAttributes(MsrXml::Element *e,
        const HRTLab::Parameter *p, bool shortReply, bool hex)
{

    // <parameter name="/lan/Control/EPC/EnableMotor/Value/2"
    //            index="30" value="0"/>
    // name=
    // value=
    // index=
    e->setAttribute("name", p->path);
    e->setAttribute("index", p->index);
    if (hex) {
        e->setAttribute("hexvalue", toHexDec(p, p->Variable::addr));
    }
    else {
        e->setAttribute("value", toCSV(p, p->Variable::addr));
    }
    if (shortReply)
        return;

    // datasize=
    // flags= Add 0x100 for dependent variables
    // mtime=
    // typ=
    e->setAttribute("datasize", p->width);
    e->setAttribute("flags", "3");
    e->setAttribute("mtime", p->getMtime());
    e->setAttribute("typ", getDTypeName(p));

    // unit=
    if (p->unit.size()) {
        e->setAttribute("unit", p->unit);
    }

    // text=
    if (p->comment.size()) {
        e->setAttribute("text", p->comment);
    }

    // For vectors:
    // anz=
    // cnum=
    // rnum=
    // orientation=
    if (p->nelem > 1) {
        e->setAttribute("anz",p->nelem);
        const char *orientation;
        size_t cnum, rnum;
        switch (p->ndims) {
            case 1:
                {
                    cnum = p->nelem;
                    rnum = 1;
                    orientation = "VECTOR";
                }
                break;

            case 2:
                {
                    const size_t *dim = p->getDim();
                    cnum = dim[1];
                    rnum = dim[0];
                    orientation = "MATRIX_ROW_MAJOR";
                }
                break;

            default:
                {
                    const size_t *dim = p->getDim();
                    cnum = dim[p->ndims - 1];
                    rnum = p->nelem / cnum;
                    orientation = "MATRIX_ROW_MAJOR";
                }
                break;
        }
        e->setAttribute("cnum", cnum);
        e->setAttribute("rnum", rnum);
        e->setAttribute("orientation", orientation);
    }

    // hide=
    // unhide=
    // persistent=
}

/////////////////////////////////////////////////////////////////////////////
void Session::parameterChanged(const HRTLab::Parameter *p)
{
    MsrXml::Element pu("pu");
    pu.setAttribute("index", p->index);

    *this << pu << std::flush;
}

/////////////////////////////////////////////////////////////////////////////
void Session::setChannelAttributes(MsrXml::Element *e,
        const HRTLab::Signal *s, bool shortReply)
{
    // <channel name="/lan/World Time" alias="" index="0" typ="TDBL"
    //   datasize="8" bufsize="500" HZ="50" unit="" value="1283134199.93743"/>
    //
    // name=
    // value=
    // index=
    e->setAttribute("name", s->path);
    e->setAttribute("index", s->index);
    if (shortReply)
        return;

    // datasize=
    // typ=
    // bufsize=
    e->setAttribute("datasize", s->width);
    e->setAttribute("typ", getDTypeName(s));
    double freq = main->baserate / s->decimation
            / (main->decimation ? main->decimation[s->tid] : 1);
    e->setAttribute("bufsize", std::max(1, (int)(freq)));
    e->setAttribute("HZ", main->baserate / s->decimation
            / (main->decimation ? main->decimation[s->tid] : 1));

    // unit=
    if (s->unit.size()) {
        e->setAttribute("unit", s->unit);
    }

    // text=
    if (s->comment.size()) {
        e->setAttribute("text", s->comment);
    }

    // For vectors:
    // anz=
    // cnum=
    // rnum=
    // orientation=
    if (s->nelem > 1) {
        e->setAttribute("anz",s->nelem);
        const char *orientation;
        size_t cnum, rnum;
        switch (s->ndims) {
            case 1:
                {
                    cnum = s->nelem;
                    rnum = 1;
                    orientation = "VECTOR";
                }
                break;

            case 2:
                {
                    const size_t *dim = s->getDim();
                    cnum = dim[1];
                    rnum = dim[0];
                    orientation = "MATRIX_ROW_MAJOR";
                }
                break;

            default:
                {
                    const size_t *dim = s->getDim();
                    cnum = dim[s->ndims - 1];
                    rnum = s->nelem / cnum;
                    orientation = "MATRIX_ROW_MAJOR";
                }
                break;
        }
        e->setAttribute("cnum", cnum);
        e->setAttribute("rnum", rnum);
        e->setAttribute("orientation", orientation);
    }

    // hide=
    // unhide=
    // persistent=
}

/////////////////////////////////////////////////////////////////////////////
void Session::readParamValuesCmd(const AttributeMap &attributes)
{
    MsrXml::Element param_values("param_values");
    std::string v;
    bool separator = false;
    const HRTLab::Main::ParameterList& pl = main->getParameters();

    for (HRTLab::Main::ParameterList::const_iterator it = pl.begin();
            it != pl.end(); it++) {
        if (separator) v.append(1,'|');
        v.append(toCSV(*it, (*it)->Variable::addr));
        separator = true;
    }

    param_values.setAttribute("value", v);

    *this << param_values << std::flush;
}

/////////////////////////////////////////////////////////////////////////////
void Session::writeParameterCmd(const AttributeMap &attributes)
{
    if (!writeAccess) {
        MsrXml::Element warn("warn");
        warn.setAttribute("text", "No write access");
        *this << warn << std::flush;
        return;
    }

    AttributeMap::const_iterator it;
    HRTLab::Parameter *parameter;
    size_t startindex = 0;
    const HRTLab::Main::ParameterList& pl = main->getParameters();

    if ((it = attributes.find("name")) != attributes.end()) {
        const HRTLab::Main::VariableMap& m = main->getVariableMap();
        HRTLab::Main::VariableMap::const_iterator vit = m.find(it->second);

        if (vit == m.end() or vit->second->index >= pl.size()
                or pl[vit->second->index]->path != it->second)
            return;

        parameter = pl[vit->second->index];
    }
    else if ((it = attributes.find("index")) != attributes.end()) {
        unsigned int index = atoi(it->second.c_str());

        if (index >= pl.size())
            return;

        parameter = pl[index];
    }
    else {
        return;
    }

    if ((it = attributes.find("startindex")) != attributes.end()) {
        startindex = atoi(it->second.c_str());
    }

    char valbuf[parameter->memSize];
    size_t validx = 0;
    if ((it = attributes.find("hexvalue")) != attributes.end()) {
        const std::string &s = it->second;
        const char hexNum[] = {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,
            0,10,11,12,13,14,15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0,10,11,12,13,14,15
        };

        for (validx = 0;
                validx < std::min(parameter->memSize, s.size() / 2);
                validx++) {
            unsigned char c1 = s[validx*2] - '0';
            unsigned char c2 = s[validx*2 + 1] - '0';
            if (c1 > 'f' - '0' or c2 > 'f' - '0')
                return;
            valbuf[validx] = hexNum[c1] << 4 | hexNum[c2];
        }
    }
    else if ((it = attributes.find("value")) != attributes.end()) {
        double v;
        char c;
        std::istringstream is(it->second);

        is.imbue(std::locale::classic());

        for (validx = 0; validx < parameter->nelem; validx++) {
            is >> v;
            cout << "value = " << v << endl;

            if (!is)
                break;

            switch (parameter->dtype) {
                case si_boolean_T:
                    reinterpret_cast<bool*>(valbuf)[validx] = v; break;

                case si_uint8_T:
                    reinterpret_cast<uint8_t*>(valbuf)[validx] = v; break;

                case si_sint8_T:
                    reinterpret_cast<int8_t*>(valbuf)[validx] = v; break;

                case si_uint16_T:
                    reinterpret_cast<uint16_t*>(valbuf)[validx] = v; break;

                case si_sint16_T:
                    reinterpret_cast<int16_t*>(valbuf)[validx] = v; break;

                case si_uint32_T:
                    reinterpret_cast<uint32_t*>(valbuf)[validx] = v; break;

                case si_sint32_T:
                    reinterpret_cast<int32_t*>(valbuf)[validx] = v; break;

                case si_uint64_T:
                    reinterpret_cast<uint64_t*>(valbuf)[validx] = v; break;

                case si_sint64_T:
                    reinterpret_cast<int64_t*>(valbuf)[validx] = v; break;

                case si_single_T:
                    reinterpret_cast<float*>(valbuf)[validx] = v; break;

                case si_double_T:
                    reinterpret_cast<double*>(valbuf)[validx] = v; break;

                default:
                    break;
            }

            is >> c;
        }
    }

    parameter->setValue(valbuf, validx, startindex);
}

/////////////////////////////////////////////////////////////////////////////
void Session::readChannelsCmd(const AttributeMap &attributes)
{
    AttributeMap::const_iterator it;
    HRTLab::Signal *signal = 0;
    bool shortReply = false;
    const HRTLab::Main::SignalList& sl = main->getSignals();

    if ((it = attributes.find("short"))!= attributes.end()) {
        shortReply = atoi(it->second.c_str());
    }

    if ((it = attributes.find("name")) != attributes.end()) {
        const HRTLab::Main::VariableMap& m = main->getVariableMap();
        HRTLab::Main::VariableMap::const_iterator vit = m.find(it->second);

        if (vit == m.end() or vit->second->index >= sl.size()
                or sl[vit->second->index]->path != it->second)
            return;

        signal = sl[vit->second->index];
    }
    else if ((it = attributes.find("index")) != attributes.end()) {
        unsigned int index = atoi(it->second.c_str());

        if (index >= sl.size())
            return;

        signal = sl[index];
    }

    if (signal) {
        char buf[signal->memSize];
        main->poll(&signal, 1, buf);

        MsrXml::Element channel("channel");
        setChannelAttributes(&channel, signal, shortReply);
        channel.setAttribute("value", toCSV(signal, buf));

        *this << channel;
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
            MsrXml::Element *c = channels.createChild("channel");
            setChannelAttributes(c, signal[i], shortReply);
            c->setAttribute("value", toCSV(signal[i], p));
            p += signal[i]->memSize;
        }
        *this << channels;
    }

    *this << std::flush;
}

/////////////////////////////////////////////////////////////////////////////
void Session::xsadCmd(const AttributeMap &attributes)
{
    AttributeMap::const_iterator it;
    std::list<HRTLab::Signal*> channelList;
    unsigned int reduction, blocksize, precision;
    bool base64 = false;
    const HRTLab::Main::SignalList& sl = main->getSignals();

    if ((it = attributes.find("channels")) != attributes.end()) {
        std::istringstream is(it->second);
        while (is) {
            unsigned int index;
            char comma;

            is >> index >> comma;
            if (index < sl.size())
                channelList.push_back(sl[index]);
        }
    }

    if ((it = attributes.find("reduction")) != attributes.end()) {
        reduction = atoi(it->second.c_str());
    }
    else {
        reduction = 1 / main->baserate;
    }

    if ((it = attributes.find("blocksize")) != attributes.end()) {
        blocksize = atoi(it->second.c_str());
    }
    else {
        blocksize = main->baserate;
    }

    if ((it = attributes.find("coding")) != attributes.end()) {
        if (it->second == "base64")
            base64 = true;
    }

    if ((it = attributes.find("precision")) != attributes.end()) {
        precision = atoi(it->second.c_str());
    }

    if ((it = attributes.find("sync")) != attributes.end()) {
        quiet = false;
    }

    if ((it = attributes.find("quiet")) != attributes.end()) {
        quiet = true;
    }

    const HRTLab::Signal *signals[channelList.size()];
    std::copy(channelList.begin(), channelList.end(), signals);
    for (const HRTLab::Signal **sp = signals;
            sp != signals + channelList.size(); sp++) {
        task[(*sp)->tid]->addSignal(*sp, reduction, blocksize);
    }

    main->subscribe(signals, channelList.size());

}

/////////////////////////////////////////////////////////////////////////////
void Session::xsodCmd(const AttributeMap &attributes)
{
}

/////////////////////////////////////////////////////////////////////////////
void Session::remoteHostCmd(const AttributeMap &attributes)
{
    AttributeMap::const_iterator it;

    if ((it = attributes.find("name")) != attributes.end()) {
        remote = it->second;
    }

    if ((it = attributes.find("applicationname")) != attributes.end()) {
        applicationname = it->second;
    }

    if ((it = attributes.find("access")) != attributes.end()) {
        writeAccess = it->second == "allow";

        if (writeAccess
                and (it = attributes.find("isadmin"))!= attributes.end()) {
            if (it->second == "true") {
                struct timespec ts;
                std::ostringstream os;
                server->getTime(ts);

                os << "Adminmode filp: " << so; // so is the fd and comes from
                                                // ost::Socket
                MsrXml::Element info("info");
                info.setAttribute("time", ts);
                info.setAttribute("text", os.str());
                server->broadcast(this, info);
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::readStatisticsCmd(const AttributeMap &attributes)
{
    // <clients>
    //   <client index="0" name="lansim"
    //           apname="Persistent Manager, Version: 0.3.1"
    //           countin="19908501" countout="27337577"
    //           connectedtime="1282151176.659208"/>
    //   <client index="1" .../>
    // </clients>
}

/////////////////////////////////////////////////////////////////////////////
void Session::nullCmd(const AttributeMap &attributes)
{
}

/////////////////////////////////////////////////////////////////////////////
void Session::echoCmd(const AttributeMap &attributes)
{
    AttributeMap::const_iterator it;

    if ((it = attributes.find("value")) != attributes.end()) {
        echo = it->second == "on";
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::broadcastCmd(const AttributeMap &attributes)
{
    MsrXml::Element broadcast("broadcast");
    AttributeMap::const_iterator it;
    struct timespec ts;

    server->getTime(ts);

    broadcast.setAttribute("time", ts);

    if ((it = attributes.find("action"))!= attributes.end()) {
        broadcast.setAttribute("action", it->second);
    }

    if ((it = attributes.find("text"))!= attributes.end()) {
        broadcast.setAttribute("text", it->second);
    }
    server->broadcast(this, broadcast);
}

/////////////////////////////////////////////////////////////////////////////
void Session::broadcast(Session *s, const MsrXml::Element &element)
{
    *this << element << std::flush;
}

/////////////////////////////////////////////////////////////////////////////
void Session::output()
{
    cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    ssize_t n = send(buf.c_str(), buf.length());
    if (n > 0)
        buf.erase(0,n);
    else {
        delete this;
        return;
    }

    if (buf.empty())
        setDetectOutput(false);
}

/////////////////////////////////////////////////////////////////////////////
void Session::disconnect()
{
    cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    delete this;
}

/////////////////////////////////////////////////////////////////////////////
const char *Session::getDTypeName(const HRTLab::Variable *v)
{
    switch (v->dtype) {
        case si_boolean_T: return "TCHAR";
        case si_uint8_T:   return "TUCHAR";
        case si_sint8_T:   return "TCHAR";
        case si_uint16_T:  return "TUSHORT";
        case si_sint16_T:  return "TSHORT";
        case si_uint32_T:  return "TUINT";
        case si_sint32_T:  return "TINT";
        case si_uint64_T:  return "TULINT";
        case si_sint64_T:  return "TLINT";
        case si_single_T:  return "TDBL";
        case si_double_T:  return "TFLT";
        default:           return "";
    }
}

/////////////////////////////////////////////////////////////////////////////
std::string Session::toHexDec(
        const HRTLab::Variable *v, const char* data)
{
    std::string s;
    const char *hexValue[256] = {
        "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", 
        "0A", "0B", "0C", "0D", "0E", "0F", "10", "11", "12", "13", 
        "14", "15", "16", "17", "18", "19", "1A", "1B", "1C", "1D", 
        "1E", "1F", "20", "21", "22", "23", "24", "25", "26", "27", 
        "28", "29", "2A", "2B", "2C", "2D", "2E", "2F", "30", "31", 
        "32", "33", "34", "35", "36", "37", "38", "39", "3A", "3B", 
        "3C", "3D", "3E", "3F", "40", "41", "42", "43", "44", "45", 
        "46", "47", "48", "49", "4A", "4B", "4C", "4D", "4E", "4F", 
        "50", "51", "52", "53", "54", "55", "56", "57", "58", "59", 
        "5A", "5B", "5C", "5D", "5E", "5F", "60", "61", "62", "63", 
        "64", "65", "66", "67", "68", "69", "6A", "6B", "6C", "6D", 
        "6E", "6F", "70", "71", "72", "73", "74", "75", "76", "77", 
        "78", "79", "7A", "7B", "7C", "7D", "7E", "7F", "80", "81", 
        "82", "83", "84", "85", "86", "87", "88", "89", "8A", "8B", 
        "8C", "8D", "8E", "8F", "90", "91", "92", "93", "94", "95", 
        "96", "97", "98", "99", "9A", "9B", "9C", "9D", "9E", "9F", 
        "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8", "A9", 
        "AA", "AB", "AC", "AD", "AE", "AF", "B0", "B1", "B2", "B3", 
        "B4", "B5", "B6", "B7", "B8", "B9", "BA", "BB", "BC", "BD", 
        "BE", "BF", "C0", "C1", "C2", "C3", "C4", "C5", "C6", "C7", 
        "C8", "C9", "CA", "CB", "CC", "CD", "CE", "CF", "D0", "D1", 
        "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "DA", "DB", 
        "DC", "DD", "DE", "DF", "E0", "E1", "E2", "E3", "E4", "E5", 
        "E6", "E7", "E8", "E9", "EA", "EB", "EC", "ED", "EE", "EF", 
        "F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", 
        "FA", "FB", "FC", "FD", "FE", "FF"};

    s.reserve(v->memSize*2 + 1);

    for (size_t i = 0; i < v->memSize; i++)
        s.append(hexValue[static_cast<unsigned char>(data[i])], 2);

    return s;
}

/////////////////////////////////////////////////////////////////////////////
std::string Session::toCSV( const HRTLab::Variable *v, const char* data)
{
    std::ostringstream os;
    os.imbue(std::locale::classic());

    switch (v->dtype) {
        case si_boolean_T:
            os << reinterpret_cast<const bool*>(data)[0];
            for ( size_t i = 1; i < v->nelem; i++)
                os << ',' << reinterpret_cast<const bool*>(data)[i];
            break;

        case si_uint8_T:
            os << reinterpret_cast<const uint8_t*>(data)[0];
            for ( size_t i = 1; i < v->nelem; i++)
                os << ',' << reinterpret_cast<const uint8_t*>(data)[i];
            break;

        case si_sint8_T:
            os << reinterpret_cast<const int8_t*>(data)[0];
            for ( size_t i = 1; i < v->nelem; i++)
                os << ',' << reinterpret_cast<const int8_t*>(data)[i];
            break;

        case si_uint16_T:
            os << reinterpret_cast<const uint16_t*>(data)[0];
            for ( size_t i = 1; i < v->nelem; i++)
                os << ',' << reinterpret_cast<const uint16_t*>(data)[i];
            break;

        case si_sint16_T:
            os << reinterpret_cast<const int16_t*>(data)[0];
            for ( size_t i = 1; i < v->nelem; i++)
                os << ',' << reinterpret_cast<const int16_t*>(data)[i];
            break;

        case si_uint32_T:
            os << reinterpret_cast<const uint32_t*>(data)[0];
            for ( size_t i = 1; i < v->nelem; i++)
                os << ',' << reinterpret_cast<const uint32_t*>(data)[i];
            break;

        case si_sint32_T:
            os << reinterpret_cast<const int32_t*>(data)[0];
            for ( size_t i = 1; i < v->nelem; i++)
                os << ',' << reinterpret_cast<const int32_t*>(data)[i];
            break;

        case si_uint64_T:
            os << reinterpret_cast<const uint64_t*>(data)[0];
            for ( size_t i = 1; i < v->nelem; i++)
                os << ',' << reinterpret_cast<const uint64_t*>(data)[i];
            break;

        case si_sint64_T:
            os << reinterpret_cast<const int64_t*>(data)[0];
            for ( size_t i = 1; i < v->nelem; i++)
                os << ',' << reinterpret_cast<const int64_t*>(data)[i];
            break;

        case si_single_T:
            os << reinterpret_cast<const float*>(data)[0];
            for ( size_t i = 1; i < v->nelem; i++)
                os << ',' << reinterpret_cast<const float*>(data)[i];
            break;

        case si_double_T:
            os << reinterpret_cast<const double*>(data)[0];
            for ( size_t i = 1; i < v->nelem; i++)
                os << ',' << reinterpret_cast<const double*>(data)[i];
            break;

        default:
            break;
    }

    return os.str();
}

