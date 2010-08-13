/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "TCPServer.h"
#include "Main.h"
#include "Variable.h"

#include <iostream>
#include <limits>

#include <iomanip>
using std::cout;
using std::cerr;
using std::endl;

using namespace HRTLab;

/////////////////////////////////////////////////////////////////////////////
TCPServer::TCPServer( ost::SocketService *ss,
        ost::TCPSocket &socket, Main *main):
    SocketPort(0, socket), main(main), crlf("\r\n"),
    signal_ptr_start(main->getSignalPtrStart())
{
    std::ostream s(this);
    setCompletion(false);
    sputn("Welcome\n", 8);
    s << "Welcome" << crlf
        << "Name: " << main->name << crlf
        << "Version: " << main->version << crlf
        << "Sampletime: " << main->baserate << crlf
        << "NumSt: "  << main->nst << crlf
        << "Decimations: ";
    for (unsigned int i = 0; i < main->nst - 1; i++) {
        if (i)
            s << ",";
        s << main->decimation[i];
    }
    s << crlf << std::flush;

    setTimer(1000);
    attach(ss);

    xmlchar = 0;
    xmlcharlen = 0;
    encoding = xmlFindCharEncodingHandler("ISO-8859-1");

    signal_ptr = signal_ptr_start;
    subscribers.resize(main->nst);
}

/////////////////////////////////////////////////////////////////////////////
int TCPServer::sync()
{
    setDetectOutput(true);
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
int TCPServer::overflow(int c)
{
    buf.append(1,c);
    return c;
}

/////////////////////////////////////////////////////////////////////////////
std::streamsize TCPServer::xsputn ( const char * s, std::streamsize n )
{
    buf.append(s,n);
    return n;
}

/////////////////////////////////////////////////////////////////////////////
void TCPServer::expired()
{
    cout << __func__ << endl;
    setTimer(1000);

    while (*signal_ptr) {
        switch (*signal_ptr++) {
            case Main::Restart:
                signal_ptr = signal_ptr_start;
                break;

            case Main::NewSubscriberList:
                {
                    unsigned int tid = *signal_ptr++;
                    unsigned int count = *signal_ptr++;

                    subscribers[tid].resize(count);
                    std::copy(subscribers[tid].begin(), subscribers[tid].end(),
                            signal_ptr);
                    signal_ptr += count;

                    cout << "Hurrah " << __func__ << endl;
                }
                break;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
void TCPServer::pending()
{
    cout << __func__ << endl;
    char buf[1024];

    ssize_t n = receive(buf, sizeof(buf));
    if (n <= 0) {
        delete this;
        return;
    }

    inbuf.append(buf,n);
    size_t bufptr = 0;
    bool cont = true;

    while (cont) {
        cout << "Command " << inbuf.substr(bufptr)  << endl;
        switch (state) {
            case Idle:
                {
                    size_t eol = inbuf.find('\n', bufptr);

                    if (eol == inbuf.npos) {
                        cont = false;
                    }
                    else {
                        std::string instruction(inbuf, bufptr, eol - bufptr);
                        state = ParseInstruction(instruction);
                        bufptr = eol + 1;
                    }

                    break;
                }
        }
    }

    inbuf.erase(0,bufptr);
}

/////////////////////////////////////////////////////////////////////////////
TCPServer::ParseState_t TCPServer::ParseInstruction(const std::string& s)
{
    std::istringstream is(s);
    std::string instruction;

    is >> instruction;

    if (!instruction.compare("ls")) {
        list();
    }
    else if (!instruction.compare("subscribe")) {
        unsigned int decimation;
        std::string path;

        is >> decimation;
        while (isspace(is.peek()))
            is.ignore(1);

        std::getline(is, path);

        if (!is.fail())
            main->subscribe(path);
    }

    return Idle;
}

/////////////////////////////////////////////////////////////////////////////
void TCPServer::subscribe(const std::string& path, unsigned int decimation)
{
}

/////////////////////////////////////////////////////////////////////////////
xmlChar* TCPServer::utf8(const std::string &s)
{
    int inlen, outlen;

    inlen = s.length();

    if (xmlcharlen < 2*inlen + 2) {
        xmlcharlen = 2*inlen + 2;
        delete[] xmlchar;
        xmlchar = new xmlChar[xmlcharlen];
    }
    outlen = xmlcharlen;

    int n =
        encoding->input(xmlchar, &outlen, (const unsigned char*)s.c_str(), &inlen);
    xmlchar[outlen] = '\0';
    return n >= 0 ? xmlchar : 0;
}

/////////////////////////////////////////////////////////////////////////////
#define TESTOUT(t) if(!(t)) goto out;
void TCPServer::list()
{
    int size;
    xmlChar *mem = 0;
    xmlDocPtr xmldoc;
    xmlNodePtr root, node;

    const std::map<std::string,Variable*>& variables = main->getVariableMap();

    TESTOUT(xmldoc = xmlNewDoc(BAD_CAST "1.0"));

    TESTOUT(root = xmlNewNode(NULL,(xmlChar*)"signals"));
    xmlDocSetRootElement(xmldoc,root);

    for (std::map<std::string,Variable*>::const_iterator it = variables.begin();
            it != variables.end(); it++) {
        Variable *v = it->second;

        TESTOUT(node = xmlNewNode(NULL,(xmlChar*)"signal"));
        xmlAddChild(root, node);

        TESTOUT(xmlNewProp(node, BAD_CAST "name",  utf8(v->path)));
        TESTOUT(xmlNewProp(node, BAD_CAST "alias", utf8(v->alias)));
        TESTOUT(xmlNewProp(node, BAD_CAST "unit",  BAD_CAST ""));

        const char* dtype;
        switch (v->dtype) {
            case si_boolean_T: dtype = "BOOL";   break;
            case si_uint8_T:   dtype = "UINT8";   break;
            case si_sint8_T:   dtype = "INT8";    break;
            case si_uint16_T:  dtype = "UINT16";  break;
            case si_sint16_T:  dtype = "INT16";   break;
            case si_uint32_T:  dtype = "UINT32";  break;
            case si_sint32_T:  dtype = "INT32";   break;
            case si_uint64_T:  dtype = "UINT64";  break;
            case si_sint64_T:  dtype = "INT64";   break;
            case si_single_T:  dtype = "FLOAT32"; break;
            case si_double_T:  dtype = "FLOAT64"; break;
            default:           dtype = "";        break;
        }
        TESTOUT(xmlNewProp(node,
                    BAD_CAST "datatype", BAD_CAST dtype));

        std::ostringstream d;
        for (size_t i = 0; i < v->dim.size(); i++) {
            if (i)
                d << ',';
            d << v->dim[i];
        }
        TESTOUT(xmlNewProp(node,
                    BAD_CAST "dimension", BAD_CAST d.str().c_str()));

        TESTOUT(xmlNewProp(node, BAD_CAST "permission",
                    BAD_CAST (v->decimation ? "r-r-r-" : "rwrwrw")));

        d.str(std::string());
        d << v->decimation;
        TESTOUT(xmlNewProp(node,
                    BAD_CAST "decimation", BAD_CAST d.str().c_str()));
    }

    xmlDocDumpFormatMemory(xmldoc, &mem, &size, 1);
    TESTOUT(mem);

    sputn(reinterpret_cast<const char*>(mem),size);
    pubsync();

out:
    xmlFreeDoc(xmldoc);
    xmlFree(mem);
    delete[] xmlchar;
}

/////////////////////////////////////////////////////////////////////////////
void TCPServer::output()
{
    cout << __func__ << endl;
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
void TCPServer::disconnect()
{
    cout << __func__ << endl;
    delete this;
}
