/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdserv library.
 *
 *  The pdserv library is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or (at
 *  your option) any later version.
 *
 *  The pdserv library is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with the pdserv library. If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include <streambuf>
#include <cerrno>       // ENAMETOOLONG
#include <climits>      // HOST_NAME_MAX
#include <unistd.h>     // gethostname
#include <log4cplus/ndc.h>
#include <log4cplus/loggingmacros.h>

#include "../Debug.h"

#include "../Main.h"
#include "../Task.h"
#include "../Signal.h"
#include "../Parameter.h"
#include "../DataType.h"

#include "Session.h"
#include "Server.h"
#include "Event.h"
#include "Channel.h"
#include "Parameter.h"
#include "XmlElement.h"
#include "XmlParser.h"
#include "SubscriptionManager.h"

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Session::Session( Server *server, ost::TCPSocket *socket):
    PdServ::Session(server->main),
    server(server),
    tcp(socket), xmlstream(&tcp),
    mutex(1)
{
    timeTask = 0;

    std::list<const PdServ::Task*> taskList(main->getTasks());
    subscriptionManager.reserve(taskList.size());
    for (; taskList.size(); taskList.pop_front()) {
        const PdServ::Task *task = taskList.front();

        subscriptionManager.push_back(new SubscriptionManager(this, task));

        if (!timeTask or timeTask->task->sampleTime > task->sampleTime)
            timeTask = subscriptionManager.back();
    }

    // Setup some internal variables
    writeAccess = false;
    echoOn = false;     // FIXME: echoOn is not yet implemented
    quiet = false;
    polite = false;
    aicDelay = 0;

    xmlstream.imbue(std::locale::classic());

    detach();
}

/////////////////////////////////////////////////////////////////////////////
Session::~Session()
{
    server->sessionClosed(this);

    for (SubscriptionManagerVector::iterator it = subscriptionManager.begin();
            it != subscriptionManager.end(); ++it)
        delete *it;
}

/////////////////////////////////////////////////////////////////////////////
void Session::getSessionStatistics(PdServ::SessionStatistics &stats) const
{
    std::ostringstream os;
    if (peer.empty())
        stats.remote = tcp.peer;
    else
        stats.remote = peer + " (" + tcp.peer +')';
    stats.client = client;
    stats.countIn = tcp.inBytes;
    stats.countOut = tcp.outBytes;
    stats.connectedTime = connectedTime;
}

/////////////////////////////////////////////////////////////////////////////
const struct timespec *Session::getTaskTime (size_t taskIdx) const
{
    return subscriptionManager[taskIdx]->taskTime;
}

/////////////////////////////////////////////////////////////////////////////
const PdServ::TaskStatistics *Session::getTaskStatistics (size_t taskIdx) const
{
    return subscriptionManager[taskIdx]->taskStatistics;
}

/////////////////////////////////////////////////////////////////////////////
void Session::broadcast(Session *, const struct timespec& ts,
        const std::string& action, const std::string &message)
{
    ost::SemaphoreLock lock(mutex);

    if (!polite) {
        Broadcast *b = new Broadcast;
        b->ts = ts;
        b->action = action;
        b->message = message;
        broadcastList.push_back(b);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::setAIC(const Parameter *p)
{
    ost::SemaphoreLock lock(mutex);
    if (!polite)
        aic.insert(p->mainParam);

    if (!aicDelay)
        aicDelay = 5;  // 2Hz AIC
}

/////////////////////////////////////////////////////////////////////////////
void Session::parameterChanged(const Parameter *p)
{
    ost::SemaphoreLock lock(mutex);
    if (!polite)
        changedParameter.insert(p);
}

/////////////////////////////////////////////////////////////////////////////
void Session::initial()
{
    log4cplus::getNDC().push(LOG4CPLUS_STRING_TO_TSTRING(tcp.peer));

    LOG4CPLUS_INFO_STR(server->log, LOG4CPLUS_TEXT("New session"));

    // Get the hostname
    char hostname[HOST_NAME_MAX+1];
    if (gethostname(hostname, HOST_NAME_MAX)) {
        if (errno == ENAMETOOLONG)
            hostname[HOST_NAME_MAX] = '\0';
        else
            strcpy(hostname,"unknown");
    }

    // Greet the new client
    {
        XmlElement greeting(tcp.createElement("connected"));
        XmlElement::Attribute(greeting, "name") << "MSR";
        XmlElement::Attribute(greeting, "host")
            << reinterpret_cast<const char*>(hostname);
        XmlElement::Attribute(greeting, "app") << main->name;
        XmlElement::Attribute(greeting, "appversion") << main->version;
        XmlElement::Attribute(greeting, "version") << MSR_VERSION;
        XmlElement::Attribute(greeting, "features") << MSR_FEATURES;
        XmlElement::Attribute(greeting, "recievebufsize") << 100000000;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::final()
{
    LOG4CPLUS_INFO_STR(server->log, LOG4CPLUS_TEXT("Finished session"));
    log4cplus::getNDC().remove();
}

/////////////////////////////////////////////////////////////////////////////
void Session::run()
{
    ssize_t n;
    XmlParser inbuf;

    while (*server->active) {
        if (!xmlstream.good()) {
            LOG4CPLUS_FATAL_STR(server->log,
                    LOG4CPLUS_TEXT("Error occurred in output stream"));
            return;
        }

        tcp.pubsync();

        try {
            n = tcp.read(inbuf.bufptr(), inbuf.free(), 40); // ms

            if (!n) {
                LOG4CPLUS_INFO_STR(server->log,
                        LOG4CPLUS_TEXT("Client closed connection"));
                return;
            }
        }
        catch (ost::Socket *s) {
            LOG4CPLUS_FATAL(server->log,
                    LOG4CPLUS_TEXT("Socket error ") << s->getErrorNumber());
            return;
        }
        catch (std::exception& e) {
            LOG4CPLUS_FATAL(server->log,
                    LOG4CPLUS_TEXT("Exception occurred: ")
                    << LOG4CPLUS_C_STR_TO_TSTRING(e.what()));
            return;
        }
        catch (...) {
            LOG4CPLUS_FATAL_STR(server->log,
                    LOG4CPLUS_TEXT("Aborting on unknown exception"));
            return;
        }

        if (n > 0) {
            LOG4CPLUS_TRACE(server->log,
                    LOG4CPLUS_TEXT("Rx: ")
                    << LOG4CPLUS_STRING_TO_TSTRING(
                        std::string(inbuf.bufptr(), n)));

            inbuf.newData(n);
            XmlParser::Element command;

            while ((command = inbuf.nextElement())) {
                command.getString("id", tcp.commandId);
                processCommand(command);

                if (!tcp.commandId.empty()) {
                    XmlElement ack(tcp.createElement("ack"));
                    XmlElement::Attribute(ack,"id")
                        .setEscaped(tcp.commandId);

                    tcp.commandId.clear();
                }
            }
        }

        // Collect all asynchronous events while holding mutex
        ParameterSet cp;
        BroadcastList broadcastList;
        {
            // Create an environment for mutex lock. This lock should be kept
            // as short as possible, and especially not when writing to the
            // output stream
            ost::SemaphoreLock lock(mutex);

            if (aicDelay)
                --aicDelay;

            ParameterSet::iterator it2, it = changedParameter.begin();
            while (it != changedParameter.end()) {
                it2 = it++;
                if (!aicDelay or aic.find((*it2)->mainParam) == aic.end()) {
                    cp.insert(*it2);
                    changedParameter.erase(it2);
                }
            }

            std::swap(this->broadcastList, broadcastList);
        }

        // Write all asynchronous events to the client
        {
            for ( ParameterSet::iterator it = cp.begin();
                    it != cp.end(); ++it) {
                XmlElement pu(tcp.createElement("pu"));
                XmlElement::Attribute(pu, "index") << (*it)->index;
            }

            for ( BroadcastList::const_iterator it = broadcastList.begin();
                    it != broadcastList.end(); ++it) {

                XmlElement broadcast(tcp.createElement("broadcast"));

                XmlElement::Attribute(broadcast, "time") << (*it)->ts;

                if (!(*it)->action.empty())
                    XmlElement::Attribute(broadcast, "action")
                        .setEscaped((*it)->action);

                if (!(*it)->message.empty())
                    XmlElement::Attribute(broadcast, "text")
                        .setEscaped((*it)->message);

                delete *it;
            }
        }

        for (SubscriptionManagerVector::iterator it = subscriptionManager.begin();
                it != subscriptionManager.end(); ++it)
            (*it)->rxPdo(tcp, quiet);

        PdServ::EventData e;
        do {
            e = main->getNextEvent(this);
        } while (Event::toXml(tcp, e));
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::processCommand(const XmlParser::Element& cmd)
{
    const char *command = cmd.getCommand();
    size_t commandLen = strlen(command);

    static const struct {
        size_t len;
        const char *name;
        void (Session::*func)(const XmlParser::Element&);
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
        { 4, "list",                    &Session::listDirectory         },
        { 9, "broadcast",               &Session::broadcast             },
        {11, "remote_host",             &Session::remoteHost            },
        {12, "read_kanaele",            &Session::readChannel           },
        {12, "read_statics",            &Session::readStatistics        },
        {14, "read_parameter",          &Session::readParameter         },
        {15, "read_statistics",         &Session::readStatistics        },
        {15, "message_history",         &Session::messageHistory        },
        {15, "write_parameter",         &Session::writeParameter        },
        {17, "read_param_values",       &Session::readParamValues       },
        {0,  0,                         0},
    };

    // Go through the command list
    for (size_t idx = 0; cmds[idx].len; idx++) {
        // Check whether the lengths fit and the string matches
        if (commandLen == cmds[idx].len
                and !strcmp(cmds[idx].name, command)) {

            LOG4CPLUS_TRACE_STR(server->log,
                    LOG4CPLUS_C_STR_TO_TSTRING(cmds[idx].name));

            // Call the method
            (this->*cmds[idx].func)(cmd);

            // Finished
            return;
        }
    }

    LOG4CPLUS_WARN(server->log,
            LOG4CPLUS_TEXT("Unknown command <")
            << LOG4CPLUS_C_STR_TO_TSTRING(command)
            << LOG4CPLUS_TEXT(">"));


    // Unknown command warning
    XmlElement warn(tcp.createElement("warn"));
    XmlElement::Attribute(warn, "num") << 1000;
    XmlElement::Attribute(warn, "text") << "unknown command";
    XmlElement::Attribute(warn, "command").setEscaped(command);
}

/////////////////////////////////////////////////////////////////////////////
void Session::broadcast(const XmlParser::Element& cmd)
{
    struct timespec ts;
    std::string action, text;

    main->gettime(&ts);
    cmd.getString("action", action);
    cmd.getString("text", text);

    server->broadcast(this, ts, action, text);
}

/////////////////////////////////////////////////////////////////////////////
void Session::echo(const XmlParser::Element& cmd)
{
    echoOn = cmd.isTrue("value");
}

/////////////////////////////////////////////////////////////////////////////
void Session::ping(const XmlParser::Element& /*cmd*/)
{
    tcp.createElement("ping");
}

/////////////////////////////////////////////////////////////////////////////
void Session::readChannel(const XmlParser::Element& cmd)
{
    const Channel *c = 0;
    bool shortReply = cmd.isTrue("short");
    std::string name;
    unsigned int index;

    if (cmd.getString("name", name)) {
        c = server->find<Channel>(name);
        if (!c)
            return;
    }
    else if (cmd.getUnsigned("index", index)) {
        c = server->getChannel(index);
        if (!c)
            return;
    }

    // A single signal was requested
    if (c) {
        char buf[c->signal->memSize];
        struct timespec time;
        int rv = static_cast<const PdServ::Variable*>(c->signal)
            ->getValue(this, buf, &time);

        XmlElement channel(tcp.createElement("channel"));
        c->setXmlAttributes(channel, shortReply, rv ? 0 : buf, 16, &time);

        return;
    }

    // A list of all channels
    const Server::Channels& chanList = server->getChannels();
    XmlElement channels(tcp.createElement("channels"));
    for (Server::Channels::const_iterator it = chanList.begin();
            it != chanList.end(); it++) {
        if ((*it)->hidden)
            continue;

        XmlElement el(channels.createChild("channel"));
        (*it)->setXmlAttributes( el, shortReply, 0, 16, 0);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::listDirectory(const XmlParser::Element& cmd)
{
    const char *path;

    if (!cmd.find("path", &path))
        return;

    XmlElement element(tcp.createElement("listing"));
    server->listDir(this, element, path);
}

/////////////////////////////////////////////////////////////////////////////
void Session::readParameter(const XmlParser::Element& cmd)
{
    bool shortReply = cmd.isTrue("short");
    bool hex = cmd.isTrue("hex");
    std::string name;
    unsigned int index;

    const Parameter *p = 0;
    if (cmd.getString("name", name)) {
        p = server->find<Parameter>(name);
        if (!p)
            return;
    }
    else if (cmd.getUnsigned("index", index)) {
        p = server->getParameter(index);
        if (!p)
            return;
    }

    if (p) {
        char buf[p->mainParam->memSize];
        struct timespec ts;

        p->mainParam->getValue(this, buf, &ts);

        std::string id;
        cmd.getString("id", id);

        XmlElement xml(tcp.createElement("parameter"));
        p->setXmlAttributes(xml, buf, ts, shortReply, hex, 16);

        return;
    }

    XmlElement parametersElement(tcp.createElement("parameters"));

    const Server::Parameters& parameters = server->getParameters();
    Server::Parameters::const_iterator it = parameters.begin();
    while ( it != parameters.end()) {
        const PdServ::Parameter* mainParam = (*it)->mainParam;
        char buf[mainParam->memSize];
        struct timespec ts;

        if ((*it)->hidden) {
            ++it;
            continue;
        }

        mainParam->getValue(this, buf, &ts);

        while (it != parameters.end() and mainParam == (*it)->mainParam) {
            XmlElement xml(parametersElement.createChild("parameter"));
            (*it++)->setXmlAttributes(xml, buf, ts, shortReply, hex, 16);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::readParamValues(const XmlParser::Element& /*cmd*/)
{
    XmlElement param_values(tcp.createElement("param_values"));
    XmlElement::Attribute values(param_values, "value");

    const Server::Parameters& parameters = server->getParameters();
    Server::Parameters::const_iterator it = parameters.begin();
    while ( it != parameters.end()) {
        const PdServ::Parameter* mainParam = (*it)->mainParam;
        char buf[mainParam->memSize];
        struct timespec ts;

        mainParam->getValue(this, buf, &ts);

        if (it != parameters.begin())
            values << ';';
        values.csv(*it, buf, 1, 16);

        while (it != parameters.end() and mainParam == (*it)->mainParam)
            ++it;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::messageHistory(const XmlParser::Element& /*cmd*/)
{
    std::list<PdServ::EventData> list(main->getEventHistory(this));

    while (!list.empty()) {
        Event::toXml(tcp, list.front());
        list.pop_front();
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::readStatistics(const XmlParser::Element& /*cmd*/)
{
    // <clients>
    //   <client index="0" name="lansim"
    //           apname="Persistent Manager, Version: 0.3.1"
    //           countin="19908501" countout="27337577"
    //           connectedtime="1282151176.659208"/>
    //   <client index="1" .../>
    // </clients>
    typedef std::list<PdServ::SessionStatistics> StatList;
    StatList stats;
    server->getSessionStatistics(stats);

    XmlElement clients(tcp.createElement("clients"));
    for (StatList::const_iterator it = stats.begin();
            it != stats.end(); it++) {
        XmlElement client(clients.createChild("client"));
        XmlElement::Attribute(client,"name")
            .setEscaped((*it).remote.size() ? (*it).remote : "unknown");
        XmlElement::Attribute(client,"apname")
            .setEscaped((*it).client.size() ? (*it).client : "unknown");
        XmlElement::Attribute(client,"countin") << (*it).countIn;
        XmlElement::Attribute(client,"countout") << (*it).countOut;
        XmlElement::Attribute(client,"connectedtime") << (*it).connectedTime;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::remoteHost(const XmlParser::Element& cmd)
{

    cmd.getString("name", peer);

    cmd.getString("applicationname", client);

    writeAccess = cmd.isEqual("access", "allow") or cmd.isTrue("access");

    // Check whether stream should be polite, i.e. not send any data
    // when not requested by the client.
    // This is used for passive clients that do not check their streams
    // on a regular basis causing the TCP stream to congest.
    {
        ost::SemaphoreLock lock(mutex);
        polite = cmd.isTrue("polite");
        if (polite) {
            changedParameter.clear();
            aic.clear();
            while (!broadcastList.empty()) {
                delete broadcastList.front();
                broadcastList.pop_front();
            }
        }
    }

    LOG4CPLUS_INFO(server->log,
            LOG4CPLUS_TEXT("Logging in ")
            << LOG4CPLUS_STRING_TO_TSTRING(peer)
            << LOG4CPLUS_TEXT(" application ")
            << LOG4CPLUS_STRING_TO_TSTRING(client)
            << LOG4CPLUS_TEXT(" writeaccess=")
            << writeAccess);
}

/////////////////////////////////////////////////////////////////////////////
void Session::writeParameter(const XmlParser::Element& cmd)
{
    if (!writeAccess) {
        XmlElement warn(tcp.createElement("warn"));
        XmlElement::Attribute(warn, "text") << "No write access";
        return;
    }

    const Parameter *p = 0;

    unsigned int index;
    std::string name;
    if (cmd.getString("name", name)) {
        p = server->find<Parameter>(name);
    }
    else if (cmd.getUnsigned("index", index)) {
        p = server->getParameter(index);
    }

    if (!p)
        return;

    unsigned int startindex = 0;
    if (cmd.getUnsigned("startindex", startindex)) {
        if (startindex >= p->dim.nelem)
            return;
    }

    if (cmd.isTrue("aic"))
        server->setAic(p);

    int errnum;
    const char *s;
    if (cmd.find("hexvalue", &s)) {
        errnum = p->setHexValue(this, s, startindex);
    }
    else if (cmd.find("value", &s)) {
        errnum = p->setDoubleValue(this, s, startindex);
    }
    else
        return;

    if (errnum) {
        // If an error occurred, tell this client to reread the value
        parameterChanged(p);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::xsad(const XmlParser::Element& cmd)
{
    unsigned int reduction, blocksize, precision, group;
    bool base64 = cmd.isEqual("coding", "Base64");
    bool event = cmd.isTrue("event");
    bool foundReduction = false;
    std::list<unsigned int> indexList;
    const Server::Channels& channel = server->getChannels();

    if (cmd.isTrue("sync")) {
        for (SubscriptionManagerVector::iterator it = subscriptionManager.begin();
                it != subscriptionManager.end(); ++it)
            (*it)->sync();
        quiet = false;
    }
    else {
        // Quiet will stop all transmission of <data> tags until
        // sync is called
        quiet = cmd.isTrue("quiet");
    }

    if (!cmd.getUnsignedList("channels", indexList))
        return;

    if (cmd.getUnsigned("reduction", reduction)) {
        if (!reduction) {
            XmlElement warn(tcp.createElement("warn"));
            XmlElement::Attribute(warn, "command") << "xsad";
            XmlElement::Attribute(warn, "text")
                << "specified reduction=0, choosing reduction=1";

            reduction = 1;
        }

        foundReduction = true;
    }

    // Discover blocksize
    if (event) {
        // blocksize of zero is for event channels
        blocksize = 0;
    }
    else if (cmd.getUnsigned("blocksize", blocksize)) {
        if (!blocksize) {
            XmlElement warn(tcp.createElement("warn"));
            XmlElement::Attribute(warn, "command") << "xsad";
            XmlElement::Attribute(warn, "text")
                << "specified blocksize=0, choosing blocksize=1";

            blocksize = 1;
        }
    }
    else {
        // blocksize was not supplied, possibly human input
        blocksize = 1;
    }

    if (!cmd.getUnsigned("precision", precision))
        precision = 16;

    if (!cmd.getUnsigned("group", group))
        group = 0;

    for (std::list<unsigned int>::const_iterator it = indexList.begin();
            it != indexList.end(); it++) {
        if (*it >= channel.size())
            continue;

        const Channel *c = channel[*it];
        const PdServ::Signal *mainSignal = c->signal;

        if (event) {
            if (!foundReduction)
                // If user did not supply a reduction, limit to a
                // max of 10Hz automatically
                reduction = static_cast<unsigned>(
				0.1/mainSignal->sampleTime + 0.5);
        }
        else if (!foundReduction) {
            // Quite possibly user input; choose reduction for 1Hz
            reduction = static_cast<unsigned>(
                    1.0/mainSignal->sampleTime / blocksize + 0.5);
        }

        subscriptionManager[c->taskIdx]->subscribe(
                c, group, reduction, blocksize, base64, precision);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::xsod(const XmlParser::Element& cmd)
{
    std::list<unsigned int> intList;

    //cout << __LINE__ << "xsod: " << endl;

    if (cmd.getUnsignedList("channels", intList)) {
        const Server::Channels& channel = server->getChannels();
        unsigned int group;

        if (!cmd.getUnsigned("group", group))
            group = 0;

        for (std::list<unsigned int>::const_iterator it = intList.begin();
                it != intList.end(); it++) {
            if (*it < channel.size()) {
                size_t taskIdx = channel[*it]->taskIdx;
                subscriptionManager[taskIdx]->unsubscribe(channel[*it], group);
            }
        }
    }
    else
        for (SubscriptionManagerVector::iterator it = subscriptionManager.begin();
                it != subscriptionManager.end(); ++it)
            (*it)->clear();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Session::TCPStream::TCPStream( ost::TCPSocket *server):
    Socket(::accept(server->getSocket(), NULL, NULL)), os(this)
{
    file = 0;

    ost::tpport_t port;
    ost::IPV4Host peer = getPeer(&port);

    std::ostringstream os;
    os << peer << ':' << port;
    this->peer = os.str();

    file = ::fdopen(so, "w");
    if (!file) {
        error(errOutput, 0, errno);
        return;
    }

    // Set socket to non-blocking mode
    //setCompletion(false);

    Socket::state = CONNECTED;
    inBytes = 0;
    outBytes = 0;
}

/////////////////////////////////////////////////////////////////////////////
Session::TCPStream::~TCPStream()
{
    ::fclose(file);
}

/////////////////////////////////////////////////////////////////////////////
XmlElement Session::TCPStream::createElement(const char* name)
{
    return XmlElement(name, os, 0, &commandId);
}

/////////////////////////////////////////////////////////////////////////////
int Session::TCPStream::read(char *buf, size_t count, timeout_t timeout)
{
    ssize_t n;
    try {
        n = readData(buf, count, 0, timeout);
    }
    catch (Socket *s) {
        if (s->getErrorNumber() != errTimeout)
            throw(s);
        setError(true);
        return -1;
    }

    inBytes += n;

    return n;
}

/////////////////////////////////////////////////////////////////////////////
int Session::TCPStream::overflow ( int c )
{
    if (::fputc(c, file) == EOF)
        return EOF;

    ++outBytes;

    return c;
}

/////////////////////////////////////////////////////////////////////////////
std::streamsize Session::TCPStream::xsputn (
        const char * s, std::streamsize count)
{
    if (::fwrite(s, 1, count, file) != static_cast<size_t>(count))
        return 0;

    outBytes += count;

    return count;
}

/////////////////////////////////////////////////////////////////////////////
int Session::TCPStream::sync ()
{
    return ::fflush(file);
}
