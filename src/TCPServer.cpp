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
    SocketPort(0, socket), main(main), crlf("\r\n")
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

    attach(ss);

    xmlchar = 0;
    xmlcharlen = 0;
    encoding = xmlFindCharEncodingHandler("ISO-8859-1");

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
    std::istringstream is(inbuf);

    while (is and is.tellg() != inbuf.length())
        ParseInstruction(is);

    inbuf.erase(0,is.tellg());

    cout << "Command3 " << inbuf << endl;
}

/////////////////////////////////////////////////////////////////////////////
void TCPServer::ParseInstruction(std::istream& is)
{
    std::string instruction;

    is >> instruction;

    cout << "Instyructuion " << instruction << endl;

    if (!instruction.compare("ls")) {
        list();
    }
    else if (!instruction.compare("login")) {
        //login(is);
    }

    is.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
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
