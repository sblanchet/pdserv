/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "MsrSession.h"
#include "Main.h"
#include "Variable.h"
#include "Parameter.h"
#include "Signal.h"
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
Session::Session( ost::SocketService *ss,
        ost::TCPSocket &socket, HRTLab::Main *main):
    SocketPort(0, socket), std::ostream(this),
    main(main), signal_ptr_start(main->getSignalPtrStart())
{
    if (commandMap.empty()) {
        commandMap["ping"] = &Session::pingCmd;
        commandMap["rp"] = &Session::readParameterCmd;
        commandMap["read_parameter"] = &Session::readParameterCmd;

        commandMap["read_param_values"] = &Session::readParamValuesCmd;
        commandMap["rpv"] = &Session::readParamValuesCmd;
        commandMap["wp"] = &Session::writeParameterCmd;
        commandMap["write_parameter"] = &Session::writeParameterCmd;
        commandMap["rk"] = &Session::readChannelsCmd;
        commandMap["rc"] = &Session::readChannelsCmd;
        commandMap["read_kanaele"] = &Session::readChannelsCmd;
        commandMap["start_data"] = &Session::xsadCmd;
        commandMap["stop_data"] = &Session::xsadCmd;
        commandMap["sad"] = &Session::xsadCmd;
        commandMap["sod"] = &Session::xsodCmd;
        commandMap["xsad"] = &Session::xsadCmd;
        commandMap["xsod"] = &Session::xsodCmd;
        commandMap["remote_host"] = &Session::remoteHostCmd;
        commandMap["broadcast"] = &Session::broadcastCmd;
        commandMap["echo"] = &Session::broadcastCmd;
        commandMap["read_statics"] = &Session::broadcastCmd;
        commandMap["rs"] = &Session::broadcastCmd;
        commandMap["te"] = &Session::broadcastCmd; //triggerevents
    }


    setCompletion(false);

    MsrXml::Element greeting("connected");

    greeting.setAttribute("name", "MSR");
    greeting.setAttribute("host", "localhost");
    greeting.setAttribute("version", MSR_VERSION);
    greeting.setAttribute("features", MSR_FEATURES);
    greeting.setAttribute("recievebufsize", "100000000");

    *this << greeting << std::flush;

    attach(ss);

    signal_ptr = signal_ptr_start;

    expired();
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
void Session::expired()
{
    while (*signal_ptr) {
        if (*signal_ptr == HRTLab::Main::Restart) {
            signal_ptr = signal_ptr_start;
            continue;
        }

        const size_t blockLen = signal_ptr[1];

        switch (*signal_ptr) {
            case HRTLab::Main::NewSubscriberList:
                {
                    size_t headerLen = 4;
                    size_t n = blockLen - headerLen;
                    unsigned int tid = signal_ptr[2];
                    //unsigned int decimation = signal_ptr[3];
//                    size_t offset = 0;

                    cout << "Main::NewSubscriberList " << tid << ' ' << n << endl;

                    for (unsigned int i = 0; i < n; i++) {
//                        size_t sigIdx = signal_ptr[i + headerLen];
//                        HRTLab::Signal *s = signals[sigIdx];
//
//                        dataOffset[sigIdx] = offset;
//                        offset += s->memSize;
//
//                        for (std::set<size_t>::iterator it = signalDecimation[s].begin();
//                                it != signalDecimation[s].end(); it++) {
//                            subscribed[tid][*it].insert(s);
//                            dirty[tid][*it] = true;
//                        }
                    }
                }
                break;

            case HRTLab::Main::SubscriptionData:
                {
//                    unsigned int tid = signal_ptr[2];
//                    //unsigned int iterationNo = signal_ptr[3];
//                    char *dataPtr = reinterpret_cast<char*>(&signal_ptr[4]);
//
//                    struct timespec time =
//                        *reinterpret_cast<struct timespec*>(dataPtr);
//                    dataPtr += sizeof(struct timespec);
//
//                    cout << "Main::SubscriptionData " << tid << endl;
//
//                    for (DecimationMap::iterator it = decimation[tid].begin();
//                            it != decimation[tid].end(); it++) {
//                        cout << tid << ' ' << it->first << ' ' << it->second << endl;
//                        if (--it->second)
//                            continue;
//
//                        it->second = it->first;
//
//                        std::ostream s(this);
//
//                        if (dirty[tid][it->first]) {
//                            s << "VARIABLELIST " << tid
//                                << ' ' << it->first
//                                << ' ' << subscribed[tid][it->first].size()
//                                << crlf;
//                            for (std::set<HRTLab::Signal*>::iterator it2 =
//                                    subscribed[tid][it->first].begin();
//                                    it2 != subscribed[tid][it->first].end();
//                                    it2++) {
//                                HRTLab::Variable *v = *it2;
//
//                                s << v->index;
//                                if (sent[v]) {
//                                    s << crlf;
//                                    continue;
//                                }
//                                sent[v] = true;
//
//                                s << ' ' << v->decimation
//                                    << ' ' << getDTypeName(v->dtype)
//                                    << ' ' << v->ndims;
//
//                                const size_t *dim = v->getDim();
//                                for (size_t i = 0; i < v->ndims; i++) {
//                                    s << ' ' << dim[i];
//                                }
//                                s << ' ' << v->alias
//                                    << ' ' << v->path
//                                    << crlf;
//
//                            }
//                            dirty[tid][it->first] = false;
//                        }
//
//                        s << "TID " << tid
//                            << ' ' << it->first
//                            << ' ' << time.tv_sec
//                            << ' ' << time.tv_nsec
//                            << crlf << std::flush;
//
//                        for (std::set<HRTLab::Signal*>::iterator it2 =
//                                subscribed[tid][it->first].begin();
//                                it2 != subscribed[tid][it->first].end();
//                                it2++) {
//                            printVariable(this, *it2,
//                                    dataPtr + dataOffset[(*it2)->index]);
//                        }
//                    }
                }
                break;
        }

        signal_ptr += blockLen;
    }

    sync();
    setTimer(100);
}

/////////////////////////////////////////////////////////////////////////////
void Session::pending()
{
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

        if (name.empty())
            break;

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
        ack.setAttribute("id", id.c_str());
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
        if (pptr+++len == eptr)
            return true;
    }
    value = std::string(pptr, len);
    pptr += len + 1;

    return false;
}

/////////////////////////////////////////////////////////////////////////////
bool Session::evalIdentifier(const char* &pptr, const char *eptr)
{
    if (!isalpha(*pptr))
        return false;

    while (isalnum(*pptr)) {
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
    const HRTLab::Main::ParameterList& pl = main->getParameters();

    if ((it = attributes.find("short"))!= attributes.end()) {
        shortReply = atoi(it->second.c_str());
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
        setParameterAttributes(&p, parameter, shortReply);
        *this << p << std::flush;
    }
    else {
        MsrXml::Element parameters("parameters");
        for (HRTLab::Main::ParameterList::const_iterator it = pl.begin();
                it != pl.end(); it++) {
            MsrXml::Element *p = parameters.createChild("parameter");
            setParameterAttributes(p, *it, shortReply);
        }
        *this << parameters << std::flush;
    }
    return;
}

/////////////////////////////////////////////////////////////////////////////
void Session::setParameterAttributes(MsrXml::Element *e,
        const HRTLab::Parameter *p, bool shortReply)
{

    // <parameter name="/lan/Control/EPC/EnableMotor/Value/2"
    //            index="30" value="0"/>
    // name=
    // value=
    // index=
    e->setAttribute("name", p->path.c_str());
    e->setAttribute("index", p->index);
    e->setAttribute("value", "");
    if (shortReply)
        return;

    // datasize=
    // flags=
    // mtime=
    // typ=
    e->setAttribute("datasize", p->width);
    e->setAttribute("flags", "259");
    e->setAttribute("mtime", "0.0");
    e->setAttribute("typ", getDTypeName(p));

    // unit=
    if (p->unit.size()) {
        e->setAttribute("unit", p->unit.c_str());
    }

    // text=
    if (p->comment.size()) {
        e->setAttribute("text", p->comment.c_str());
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
void Session::readParamValuesCmd(const AttributeMap &attributes)
{
}

/////////////////////////////////////////////////////////////////////////////
void Session::writeParameterCmd(const AttributeMap &attributes)
{
}

/////////////////////////////////////////////////////////////////////////////
void Session::readChannelsCmd(const AttributeMap &attributes)
{
}

/////////////////////////////////////////////////////////////////////////////
void Session::xsadCmd(const AttributeMap &attributes)
{
}

/////////////////////////////////////////////////////////////////////////////
void Session::xsodCmd(const AttributeMap &attributes)
{
}

/////////////////////////////////////////////////////////////////////////////
void Session::remoteHostCmd(const AttributeMap &attributes)
{
}

/////////////////////////////////////////////////////////////////////////////
void Session::broadcastCmd(const AttributeMap &attributes)
{
}

/////////////////////////////////////////////////////////////////////////////
void Session::output()
{
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
    cout << __func__ << endl;
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
