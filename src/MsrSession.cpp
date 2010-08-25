/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "MsrSession.h"
#include "Main.h"
#include "Variable.h"
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
        commandMap["ping"] = &Session::evalPing;
        commandMap["rp"] = &Session::evalPing;
        commandMap["read_parameter"] = &Session::evalPing;
        commandMap["read_param_values"] = &Session::evalPing;
        commandMap["wp"] = &Session::evalPing;
        commandMap["write_parameter"] = &Session::evalPing;
        commandMap["rk"] = &Session::evalPing;
        commandMap["read_kanaele"] = &Session::evalPing;
        commandMap["xsad"] = &Session::evalPing;
        commandMap["xsod"] = &Session::evalPing;
        commandMap["remote_host"] = &Session::evalPing;
        commandMap["broadcast"] = &Session::evalPing;
    }

    setCompletion(false);

    MsrXml::Element greeting("connected");

    std::ostringstream ver;
    ver << MSR_VERSION;

    greeting.setAttribute("name", "MSR");
    greeting.setAttribute("host", "localhost");
    greeting.setAttribute("version", ver.str().c_str());
    greeting.setAttribute("features", MSR_FEATURES);
    greeting.setAttribute("recievebufsize", "10000000");

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
    std::list<AttributeTuple> attributes;

    std::string id;
    while (true) {
        AttributeTuple a;
        if (evalAttribute(pptr, eptr, a))
            return true;

        if (!a.name)
            break;

        if (!strncmp("id", a.name, a.nameLen))
            id = std::string(a.value, a.valueLen);

        attributes.push_back(a);
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
    if (it != commandMap.end()) {
        (this->*(it->second))(attributes);
    }
    else if (!strncmp(cmd, "ping", cmdLen)) {
        evalPing(attributes);
    }
    else {
        MsrXml::Element warn("warn");
        warn.setAttribute("num","1000");
        warn.setAttribute("text", "unknown command");
        warn.setAttribute("command", cmd, cmdLen);
        *this << warn << std::flush;

        return false;
    }

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
        AttributeTuple &a)
{
    cout << __LINE__ << __func__ << endl;
    a.name = 0;

    const char *start = pptr;
    while (isspace(*pptr)) 
        if (++pptr == eptr)
            return true;
    cout << __LINE__ << __func__ << endl;

    if (pptr == start) {
        // No whitespace found
        return false;
    }
    cout << __LINE__ << __func__ << endl;

    start = pptr;
    if (evalIdentifier(pptr, eptr))
        return true;
    cout << __LINE__ << __func__ << endl;

    if (eptr - pptr < 3)
        return true;
    cout << __LINE__ << __func__ << endl;

    a.nameLen = pptr - start;
    if (*pptr++ != '=' or *pptr++ != '"')
        return false;

    cout << __LINE__ << __func__ << endl;
    a.name = start;

    cout << __LINE__ << __func__ << endl;
    a.value = pptr;
    while (true) {
    cout << __LINE__ << __func__ << endl;
        if (pptr >= eptr)
            return true;
        if (*pptr == '\\') {
            pptr += 2;
            continue;
        }
        if (*pptr++ == '"')
            break;
    }
    cout << __LINE__ << __func__ << endl;

    a.valueLen = pptr - a.value - 1;
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
void Session::evalPing(const std::list<AttributeTuple> &attributes)
{
    MsrXml::Element ping("ping");
    for (AttributeList::const_iterator it = attributes.begin();
            it != attributes.end(); it++) {
        if (!strncmp("id", (*it).name, (*it).nameLen)) {
            ping.setAttribute("id", (*it).value, (*it).valueLen);
            break;
        }
    }
    *this << ping << std::flush;
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
const char *Session::getDTypeName(const enum si_datatype_t& t)
{
    switch (t) {
        case si_boolean_T: return "BOOL";
        case si_uint8_T:   return "UINT8";
        case si_sint8_T:   return "INT8";
        case si_uint16_T:  return "UINT16";
        case si_sint16_T:  return "INT16";
        case si_uint32_T:  return "UINT32";
        case si_sint32_T:  return "INT32";
        case si_uint64_T:  return "UINT64";
        case si_sint64_T:  return "INT64";
        case si_single_T:  return "FLOAT32";
        case si_double_T:  return "FLOAT64";
        default:           return "";
    }
}
