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

    for (size_t i = 0; i < main->numTasks(); ++i) {
        const PdServ::Task *task = main->getTask(i);

        subscriptionManager[task] = new SubscriptionManager(this, task);

        if (!timeTask or timeTask->task->sampleTime > task->sampleTime)
            timeTask = subscriptionManager[task];
    }

    // Setup some internal variables
    writeAccess = false;
    echoOn = false;     // FIXME: echoOn is not yet implemented
    quiet = false;
    aicDelay = 0;

    xmlstream.imbue(std::locale::classic());

    detach();
}

/////////////////////////////////////////////////////////////////////////////
Session::~Session()
{
    for (SubscriptionManagerMap::iterator it = subscriptionManager.begin();
            it != subscriptionManager.end(); ++it)
        delete it->second;

    server->sessionClosed(this);
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
const struct timespec *Session::getTaskTime (const PdServ::Task *task) const
{
    return subscriptionManager.find(task)->second->taskTime;
}

/////////////////////////////////////////////////////////////////////////////
const PdServ::TaskStatistics *Session::getTaskStatistics (
        const PdServ::Task *task) const
{
    return subscriptionManager.find(task)->second->taskStatistics;
}

/////////////////////////////////////////////////////////////////////////////
void Session::broadcast(Session *, const std::string &message)
{
    ost::SemaphoreLock lock(xmlstream.mutex);

    xmlstream << message << std::flush;
}

/////////////////////////////////////////////////////////////////////////////
void Session::setAIC(const Parameter *p)
{
    ost::SemaphoreLock lock(mutex);
    aic.insert(p->mainParam);

    if (!aicDelay)
        aicDelay = 5;  // 2Hz AIC
}

/////////////////////////////////////////////////////////////////////////////
void Session::parameterChanged(const Parameter *p)
{
    ost::SemaphoreLock lock(mutex);

    if (aic.find(p->mainParam) == aic.end()) {
        ostream::locked ls(xmlstream);
        XmlElement pu("pu", ls);
        XmlElement::Attribute(pu, "index") << p->index;
    }
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
        XmlElement greeting("connected", xmlstream);
        XmlElement::Attribute(greeting, "name") << "MSR";
        XmlElement::Attribute(greeting, "host")
            << reinterpret_cast<const char*>(hostname);
        XmlElement::Attribute(greeting, "app") << main->name;
        XmlElement::Attribute(greeting, "appversion") << main->version;
        XmlElement::Attribute(greeting, "version") << MSR_VERSION;
        XmlElement::Attribute(greeting, "features") << MSR_FEATURES;
        XmlElement::Attribute(greeting, "recievebufsize") << 100000000;
    }

    xmlstream.mutex.post();
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

    while (xmlstream.good()) {
        try {
            n = tcp.read(inbuf.bufptr(), inbuf.free(), 100);

            if (!n) {
                LOG4CPLUS_DEBUG_STR(server->log,
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
            LOG4CPLUS_DEBUG(server->log,
                    LOG4CPLUS_TEXT("Rx: ")
                    << LOG4CPLUS_STRING_TO_TSTRING(
                        std::string(inbuf.bufptr(), n)));

            inbuf.newData(n);
            XmlParser::Element command;

            while ((command = inbuf.nextElement()))
                processCommand(command);
        }

        for (SubscriptionManagerMap::iterator it = subscriptionManager.begin();
                it != subscriptionManager.end(); ++it) {
            it->second->rxPdo(xmlstream, quiet);
        }

        ost::SemaphoreLock lock(mutex);
        if (aicDelay and !--aicDelay) {
            for ( AicSet::iterator it = aic.begin();
                    it != aic.end(); ++it) {
                parameterChanged(server->find(*it));
            }

            aic.clear();
        }

        const PdServ::Event* mainEvent;
        bool state;
        struct timespec t;
        size_t index;
        while ((mainEvent = main->getNextEvent(this, &index, &state, &t))) {
            ostream::locked ls(xmlstream);
            Event::toXml(ls, mainEvent, index, state, t);
        }
    }

    LOG4CPLUS_FATAL_STR(server->log,
            LOG4CPLUS_TEXT("Error occurred in output stream"));
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
        {15, "write_parameter",         &Session::writeParameter        },
        {17, "read_param_values",       &Session::readParamValues       },
        {0,},
    };

    // Go through the command list
    for (size_t idx = 0; cmds[idx].len; idx++) {
        // Check whether the lengths fit and the string matches
        if (commandLen == cmds[idx].len
                and !strcmp(cmds[idx].name, command)) {

            LOG4CPLUS_TRACE(server->log,
                    LOG4CPLUS_C_STR_TO_TSTRING(cmds[idx].name));

            // Call the method
            (this->*cmds[idx].func)(cmd);

            // If "ack" attribute was set, send it back
            std::string id;
            if (cmd.getString("id", id)) {
                ostream::locked ls(xmlstream);
                XmlElement ack("ack", ls);
                XmlElement::Attribute(ack,"id").setEscaped(id.c_str());
            }

            // Finished
            return;
        }
    }

    LOG4CPLUS_WARN(server->log,
            LOG4CPLUS_TEXT("Unknown command <")
            << LOG4CPLUS_C_STR_TO_TSTRING(command)
            << LOG4CPLUS_TEXT(">"));


    // Unknown command warning
    ostream::locked ls(xmlstream);
    XmlElement warn("warn", ls);
    XmlElement::Attribute(warn, "num") << 1000;
    XmlElement::Attribute(warn, "text") << "unknown command";
    XmlElement::Attribute(warn, "command").setEscaped(command);
}

/////////////////////////////////////////////////////////////////////////////
void Session::broadcast(const XmlParser::Element& cmd)
{
    std::ostringstream os;
    {
        XmlElement broadcast("broadcast", os);
        struct timespec ts;
        std::string s;

        main->gettime(&ts);
        XmlElement::Attribute(broadcast, "time") << ts;

        if (cmd.getString("action", s))
            XmlElement::Attribute(broadcast, "action").setEscaped(s.c_str());

        if (cmd.getString("text", s))
            XmlElement::Attribute(broadcast, "text").setEscaped(s.c_str());
    }

    server->broadcast(this, os.str());
}

/////////////////////////////////////////////////////////////////////////////
void Session::echo(const XmlParser::Element& cmd)
{
    echoOn = cmd.isTrue("value");
}

/////////////////////////////////////////////////////////////////////////////
void Session::ping(const XmlParser::Element& cmd)
{
    ostream::locked ls(xmlstream);
    XmlElement ping("ping", ls);
    std::string id;

    if (cmd.getString("id",id))
        XmlElement::Attribute(ping, "id").setEscaped(id.c_str());
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

        c->signal->getValue(this, buf);

        ostream::locked ls(xmlstream);
        XmlElement channel("channel", ls);
        c->setXmlAttributes(channel, shortReply, buf, 16);

        return;
    }

    size_t buflen = 0;
    const PdServ::Signal *mainSignal = 0;
    std::map<const PdServ::Signal*, size_t> bufOffset;

    const Server::Channels& chanList = server->getChannels();

    typedef std::list<const PdServ::Signal*> SignalList;
    SignalList orderedSignals[PdServ::DataType::maxWidth + 1];

    for (Server::Channels::const_iterator it = chanList.begin();
            it != chanList.end(); it++) {
        mainSignal = (*it)->signal;
        if (bufOffset.find(mainSignal) != bufOffset.end()
                or (*it)->hidden)
            continue;

        bufOffset[mainSignal] = 0;
        orderedSignals[mainSignal->dtype.align()].push_back(mainSignal);
    }

    const PdServ::Signal *signalList[bufOffset.size()];

    index = 0;
    for (size_t w = 8; w; w /= 2) {
        for (SignalList::const_iterator it = orderedSignals[w].begin();
                it != orderedSignals[w].end(); it++) {
            mainSignal = *it;

            signalList[index] = mainSignal;
            bufOffset[mainSignal] = buflen;
            buflen += mainSignal->memSize;

            index++;
        }
    }

    char buf[buflen];
    main->poll(this, signalList, bufOffset.size(), buf, 0);

    ostream::locked ls(xmlstream);
    XmlElement channels("channels", ls);
    for (Server::Channels::const_iterator it = chanList.begin();
            it != chanList.end(); it++) {
        if ((*it)->hidden)
            continue;

        XmlElement el("channel", channels);
        (*it)->setXmlAttributes(
                el, shortReply, buf + bufOffset[(*it)->signal], 16);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::listDirectory(const XmlParser::Element& cmd)
{
    const char *path;

    if (!cmd.find("path", path))
        return;

    ostream::locked ls(xmlstream);
    XmlElement element("listing", ls);
    server->list(this, element, path);
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

        ostream::locked ls(xmlstream);
        XmlElement xml("parameter", ls);
        p->setXmlAttributes(xml, buf, ts, shortReply, hex, 16, id);

        return;
    }

    ostream::locked ls(xmlstream);
    XmlElement parametersElement("parameters", ls);

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
            XmlElement xml("parameter",  parametersElement);
            (*it++)->setXmlAttributes(xml, buf, ts, shortReply, hex, 16);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::readParamValues(const XmlParser::Element& cmd)
{
    ostream::locked ls(xmlstream);
    XmlElement param_values("param_values", ls);
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
void Session::readStatistics(const XmlParser::Element& cmd)
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
    main->getSessionStatistics(stats);

    ostream::locked ls(xmlstream);
    XmlElement clients("clients", ls);
    for (StatList::const_iterator it = stats.begin();
            it != stats.end(); it++) {
        XmlElement client("client", clients);
        XmlElement::Attribute(client,"name").setEscaped(
                (*it).remote.size() ? (*it).remote.c_str() : "unknown");
        XmlElement::Attribute(client,"apname").setEscaped(
                (*it).client.size() ? (*it).client.c_str() : "unknown");
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
        ostream::locked ls(xmlstream);
        XmlElement warn("warn", ls);
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
    size_t count;
    if (cmd.find("hexvalue", s)) {
        errnum = p->setHexValue(this, s, startindex, count);
    }
    else if (cmd.find("value", s)) {
        errnum = p->setDoubleValue(this, s, startindex, count);
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
    unsigned int reduction, blocksize, precision;
    bool base64 = cmd.isEqual("coding", "Base64");
    bool event = cmd.isTrue("event");
    bool foundReduction = false, foundBlocksize = false;
    std::list<unsigned int> indexList;
    const Server::Channels& channel = server->getChannels();

    if (cmd.isTrue("sync")) {
        for (SubscriptionManagerMap::iterator it = subscriptionManager.begin();
                it != subscriptionManager.end(); ++it)
            it->second->sync();
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
            ostream::locked ls(xmlstream);
            XmlElement warn("warn", ls);
            XmlElement::Attribute(warn, "command") << "xsad";
            XmlElement::Attribute(warn, "text")
                << "specified reduction=0, choosing reduction=1";

            reduction = 1;
        }

        foundReduction = true;
    }

    if (cmd.getUnsigned("blocksize", blocksize)) {
        if (!blocksize) {
            ostream::locked ls(xmlstream);
            XmlElement warn("warn", ls);
            XmlElement::Attribute(warn, "command") << "xsad";
            XmlElement::Attribute(warn, "text")
                << "specified blocksize=0, choosing blocksize=1";

            blocksize = 1;
        }

        foundBlocksize = true;
    }

    if (!cmd.getUnsigned("precision", precision))
        precision = 16;

    for (std::list<unsigned int>::const_iterator it = indexList.begin();
            it != indexList.end(); it++) {
        if (*it >= channel.size())
            continue;

        const PdServ::Signal *mainSignal = channel[*it]->signal;

        if (event) {
            if (!foundReduction)
                // If user did not supply a reduction, limit to a
                // max of 10Hz automatically
                reduction = static_cast<unsigned>(
				0.1 / mainSignal->sampleTime
                                / mainSignal->decimation + 0.5);
        }
        else if (!foundReduction or !foundBlocksize) {
            // Quite possibly user input; choose reduction for 1Hz

            if (!foundBlocksize)
                blocksize = 1;

            if (!foundReduction)
                reduction = static_cast<unsigned>(
				1.0 / mainSignal->sampleTime
                                / mainSignal->decimation
                                / blocksize + 0.5);
        }

        double ts = mainSignal->sampleTime;
        //log_debug("Subscribe to signal %s %i %f", channel[*it]->path().c_str(),
                //*it, ts);
        subscriptionManager[main->getTask(ts)]->subscribe( channel[*it], event,
                    reduction * mainSignal->decimation,
                    blocksize, base64, precision);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::xsod(const XmlParser::Element& cmd)
{
    std::list<unsigned int> intList;

    //cout << __LINE__ << "xsod: " << endl;

    if (cmd.getUnsignedList("channels", intList)) {
        const Server::Channels& channel = server->getChannels();
        for (std::list<unsigned int>::const_iterator it = intList.begin();
                it != intList.end(); it++) {
            if (*it < channel.size()) {
                double ts = channel[*it]->signal->sampleTime;
                subscriptionManager[main->getTask(ts)]->unsubscribe(
                        channel[*it]);
            }
        }
    }
    else
        for (SubscriptionManagerMap::iterator it = subscriptionManager.begin();
                it != subscriptionManager.end(); ++it)
            it->second->clear();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Session::TCPStream::TCPStream( ost::TCPSocket *server):
    Socket(::accept(server->getSocket(), NULL, NULL))
{
    file = 0;

    ost::tpport_t port;
    ost::IPV4Host peer = getPeer(&port);

    std::ostringstream os;
    os << peer << ':' << port;
    this->peer = os.str();

    if (!server->onAccept(peer, port)) {
        error(errConnectRejected);
        return;
    }

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
int Session::TCPStream::getSocket() const
{
    return so;
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
    if (count and ::fwrite(s, 1, count, file) != static_cast<size_t>(count))
        return 0;

    outBytes += count;

    return count;
}

/////////////////////////////////////////////////////////////////////////////
int Session::TCPStream::sync ()
{
    return ::fflush(file);
}
