/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdserv package.
 *
 *  pdserv is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pdserv is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pdserv. See COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include <streambuf>
#include <cerrno>       // ENAMETOOLONG
#include <climits>      // HOST_NAME_MAX
#include <unistd.h>     // gethostname

#include <log4cpp/Category.hh>
#include <log4cpp/NDC.hh>

#include "../Debug.h"

#include "../Main.h"
#include "../Task.h"
#include "../Signal.h"
#include "../Parameter.h"
#include "../SessionShadow.h"
#include "../SessionTaskData.h"
#include "../SessionStatistics.h"
#include "../TaskStatistics.h"

#include "Session.h"
#include "Server.h"
#include "Channel.h"
#include "Parameter.h"
#include "Directory.h"
#include "SubscriptionManager.h"
#include "TaskStatistics.h"
#include "Subscription.h"

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Session::Session( Server *server, ost::TCPSocket *socket,
        log4cpp::NDC::ContextStack* ctxt):
    PdServ::Session(server->main),
    server(server), root(server->getRoot()), streamlock(1),
    tcp(socket), ostream(&tcp), mutex(1), ctxt(ctxt)
{
    for (unsigned int i = 0; i < main->numTasks(); ++i) {
        const PdServ::Task *task = main->getTask(i);

        subscriptionManager[task] = new SubscriptionManager(this);
    }

    // Setup some internal variables
    writeAccess = false;
    echoOn = false;
    quiet = false;
    sync = false;
    aicDelay = 0;

    ostream.imbue(std::locale::classic());

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
void Session::broadcast(Session *, const std::string &message)
{
    ostream << message << std::flush;
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
void Session::parameterChanged(const PdServ::Parameter *p,
        size_t startIndex, size_t nelem)
{
    const Parameter *param = root.find(p);

    ost::SemaphoreLock lock(mutex);
    if (aic.find(p) == aic.end())
        param->valueChanged(ostream, streamlock, startIndex, nelem);
}

/////////////////////////////////////////////////////////////////////////////
void Session::initial()
{
    log4cpp::NDC::inherit(ctxt);
    log4cpp::NDC::push(tcp.peer.c_str());

    server->log.notice("New session");

    // Get the hostname
    char hostname[HOST_NAME_MAX+1];
    switch (gethostname(hostname, HOST_NAME_MAX)) {
        case ENAMETOOLONG:
            hostname[HOST_NAME_MAX] = '\0';
            // no break
        case 0:
            break;

        default:
            strcpy(hostname,"unknown");
            break;
    }

    // Greet the new client
    XmlElement greeting("connected", ostream, streamlock);
    XmlElement::Attribute(greeting, "name")
        << main->name << " Version " << main->version;
    XmlElement::Attribute(greeting, "host")
        << reinterpret_cast<const char*>(hostname);
    XmlElement::Attribute(greeting, "version") << MSR_VERSION;
    XmlElement::Attribute(greeting, "features") << MSR_FEATURES;
    XmlElement::Attribute(greeting, "recievebufsize") << 100000000;

}

/////////////////////////////////////////////////////////////////////////////
void Session::final()
{
    server->log.notice("Finished session");
    log4cpp::NDC::pop();
}

/////////////////////////////////////////////////////////////////////////////
void Session::run()
{
    ssize_t n;

    try {
        while (ostream.good()) {
            n = tcp.read(inbuf.bufptr(), inbuf.free(), 100);
            server->log.debug("Received %i bytes", n);
            if (!n)
                break;
            else if (n > 0) {
                inbuf.newData(n);

                while (inbuf.next())
                    processCommand();
            }

            if (rxPdo()) {
                // Unknown command warning
                XmlElement error("error", ostream, streamlock);
                XmlElement::Attribute(error, "text")
                    << "process synchronization lost";

                sync = true;
            }

            ost::SemaphoreLock lock(mutex);
            if (aicDelay and !--aicDelay) {
                for ( AicSet::iterator it = aic.begin();
                        it != aic.end(); ++it) {
                    const Parameter *param = root.find(*it);
                    param->valueChanged(ostream, streamlock, 0, param->nelem);
                }

                aic.clear();
            }
        }
    }
    catch (ost::Socket *s) {
        server->log.crit("Socket error %i", s->getErrorNumber());
    }
    catch (std::exception& e) {
        server->log.crit("Exception occurred: %s", e.what());
    }
    catch (...) {
        server->log.crit("Aborting on unknown exception");
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::newSignal(const PdServ::Task *task, const PdServ::Signal *s)
{
    sync |= subscriptionManager[task]->newSignal(s);
}

/////////////////////////////////////////////////////////////////////////////
void Session::newSignalData(const PdServ::SessionTaskData *std,
        const struct timespec *time)
{
    if (sync) {
        sync = false;

        for (SubscriptionManagerMap::iterator it = subscriptionManager.begin();
                it != subscriptionManager.end(); ++it)
            it->second->sync();
    }
    
    SubscriptionManager::PrintQ printQueue;
    subscriptionManager[std->task]->newSignalData(printQueue, std);

    if (!printQueue.empty() and !quiet) {
        XmlElement dataTag("data", ostream, streamlock);
        XmlElement::Attribute(dataTag, "level") << 0;
        XmlElement::Attribute(dataTag, "time") << *time;

        while (!printQueue.empty()) {
            printQueue.front()->print(dataTag);
            printQueue.pop();
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::processCommand()
{
    const char *command = inbuf.getCommand();
    size_t commandLen = strlen(command);

    static const struct {
        size_t len;
        const char *name;
        void (Session::*func)();
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

            server->log.debug(cmds[idx].name);

            // Call the method
            (this->*cmds[idx].func)();

            // If "ack" attribute was set, send it back
            std::string id;
            if (inbuf.getString("id", id)) {
                XmlElement ack("ack", ostream, streamlock);
                XmlElement::Attribute(ack,"id").setEscaped(id.c_str());
            }

            // Finished
            return;
        }
    }

    // Unknown command warning
    XmlElement warn("warn", ostream, streamlock);
    XmlElement::Attribute(warn, "num") << 1000;
    XmlElement::Attribute(warn, "text") << "unknown command";
    XmlElement::Attribute(warn, "command").setEscaped(command);
}

/////////////////////////////////////////////////////////////////////////////
void Session::broadcast()
{
    std::ostringstream os;
    {
        XmlElement broadcast("broadcast", os, streamlock);
        struct timespec ts;
        std::string s;

        main->gettime(&ts);
        XmlElement::Attribute(broadcast, "time") << ts;

        if (inbuf.getString("action", s))
            XmlElement::Attribute(broadcast, "action").setEscaped(s.c_str());

        if (inbuf.getString("text", s))
            XmlElement::Attribute(broadcast, "text").setEscaped(s.c_str());
    }

    server->broadcast(this, os.str());
}

/////////////////////////////////////////////////////////////////////////////
void Session::echo()
{
    echoOn = inbuf.isTrue("value");
}

/////////////////////////////////////////////////////////////////////////////
void Session::ping()
{
    XmlElement ping("ping", ostream, streamlock);
    std::string id;

    if (inbuf.getString("id",id))
        XmlElement::Attribute(ping, "id").setEscaped(id.c_str());
}

/////////////////////////////////////////////////////////////////////////////
void Session::readChannel()
{
    const Channel *c = 0;
    bool shortReply = inbuf.isTrue("short");
    std::string path;
    unsigned int index;

    if (inbuf.getString("name", path)) {
        c = root.find<Channel>(path);
        if (!c)
            return;
    }
    else if (inbuf.getUnsigned("index", index)) {
        c = root.getChannel(index);
        if (!c)
            return;
    }

    // A single signal was requested
    if (c) {
        char buf[c->signal->memSize];

        c->signal->getValue(this, buf);

        XmlElement channel("channel", ostream, streamlock);
        c->setXmlAttributes(channel, shortReply, buf, 16);

        return;
    }

    size_t buflen = 0;
    const PdServ::Signal *mainSignal = 0;
    std::map<const PdServ::Signal*, size_t> bufOffset;

    const VariableDirectory::Channels& chanList = root.getChannels();

    typedef std::list<const PdServ::Signal*> SignalList;
    SignalList orderedSignals[PdServ::Variable::maxWidth + 1];

    for (VariableDirectory::Channels::const_iterator it = chanList.begin();
            it != chanList.end(); it++) {
        mainSignal = (*it)->signal;
        if (bufOffset.find(mainSignal) != bufOffset.end())
            continue;

        bufOffset[mainSignal] = 0;
        orderedSignals[mainSignal->elemSize].push_back(mainSignal);
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

    XmlElement channels("channels", ostream, streamlock);
    for (VariableDirectory::Channels::const_iterator it = chanList.begin();
            it != chanList.end(); it++)
        XmlElement("channel", channels).setAttributes(*it,
                buf + bufOffset[(*it)->signal], shortReply, 16);
}

/////////////////////////////////////////////////////////////////////////////
void Session::listDirectory()
{
    const char *path;

    if (!inbuf.find("path", path))
        return;

    XmlElement element("listing", ostream, streamlock);
    root.list(this, element, path);
}

/////////////////////////////////////////////////////////////////////////////
void Session::readParameter()
{
    bool shortReply = inbuf.isTrue("short");
    bool hex = inbuf.isTrue("hex");
    std::string name;
    unsigned int index;

    const Parameter *p = 0;
    if (inbuf.getString("name", name)) {
        p = root.find<Parameter>(name.c_str());
        if (!p)
            return;
    }
    else if (inbuf.getUnsigned("index", index)) {
        p = root.getParameter(index);
        if (!p)
            return;
    }

    if (p) {
        char buf[p->mainParam->memSize];
        struct timespec ts;

        p->mainParam->getValue(this, buf, &ts);

        std::string id;
        inbuf.getString("id", id);
        XmlElement ("parameter", ostream, streamlock)
            .setAttributes(p, buf, ts, shortReply, hex, 16, id);

        return;
    }

    XmlElement parametersElement("parameters", ostream, streamlock);

    const VariableDirectory::Parameters& parameters = root.getParameters();
    VariableDirectory::Parameters::const_iterator it = parameters.begin();
    while ( it != parameters.end()) {
        const PdServ::Parameter* mainParam = (*it)->mainParam;
        char buf[mainParam->memSize];
        struct timespec ts;

        mainParam->getValue(this, buf, &ts);

        while (it != parameters.end() and mainParam == (*it)->mainParam)
            XmlElement ("parameter",  parametersElement).setAttributes(
                    *it++, buf, ts, shortReply, hex, 16);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::readParamValues()
{
    XmlElement param_values("param_values", ostream, streamlock);
    XmlElement::Attribute values(param_values, "value");

    const VariableDirectory::Parameters& parameters = root.getParameters();
    VariableDirectory::Parameters::const_iterator it = parameters.begin();
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
void Session::readStatistics()
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

    XmlElement clients("clients", ostream, streamlock);
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
void Session::remoteHost()
{
    inbuf.getString("name", peer);

    inbuf.getString("applicationname", client);

    writeAccess = inbuf.isEqual("access", "allow") or inbuf.isTrue("access");

    if (writeAccess and inbuf.isTrue("isadmin")) {
        struct timespec ts;
        std::ostringstream os;
        std::ostringstream message;
        main->gettime(&ts);

        os << "Adminmode filp: " << tcp.getSocket();
        {
            XmlElement info("info", message, streamlock);
            XmlElement::Attribute(info, "time") << ts;
            XmlElement::Attribute(info, "text") << os.str();
        }

        server->broadcast(this, message.str());
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::writeParameter()
{
    if (!writeAccess) {
        XmlElement warn("warn", ostream, streamlock);
        XmlElement::Attribute(warn, "text") << "No write access";
        return;
    }

    const Parameter *p = 0;

    unsigned int index;
    std::string name;
    if (inbuf.getString("name", name)) {
        p = root.find<Parameter>(name.c_str());
    }
    else if (inbuf.getUnsigned("index", index)) {
        p = root.getParameter(index);
    }

    if (!p)
        return;
    
    unsigned int startindex = 0;
    if (inbuf.getUnsigned("startindex", startindex)) {
        if (startindex >= p->nelem)
            return;
    }

    if (inbuf.isTrue("aic"))
        server->setAic(p);

    int errnum;
    const char *s;
    size_t count;
    if (inbuf.find("hexvalue", s)) {
        errnum = p->setHexValue(this, s, startindex, count);
    }
    else if (inbuf.find("value", s)) {
        errnum = p->setDoubleValue(this, s, startindex, count);
    }
    else
        return;

    if (errnum) {
        // If an error occurred, tell this client to reread the value
        parameterChanged(p->mainParam, startindex, count);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::xsad()
{
    unsigned int reduction, blocksize, precision;
    bool base64 = inbuf.isEqual("coding", "Base64");
    bool event = inbuf.isTrue("event");
    bool sync = inbuf.isTrue("sync");
    bool foundReduction = false, foundBlocksize = false;
    std::list<unsigned int> indexList;
    const VariableDirectory::Channels& channel = root.getChannels();

    // Quiet will stop all transmission of <data> tags until
    // sync is called
    // FIXME: sync is not yet implemented correctly
    quiet = !sync and inbuf.isTrue("quiet");

    if (!inbuf.getUnsignedList("channels", indexList)) {
        this->sync = sync;
        return;
    }

    if (inbuf.getUnsigned("reduction", reduction)) {
        if (!reduction) {
            XmlElement warn("warn", ostream, streamlock);
            XmlElement::Attribute(warn, "command") << "xsad";
            XmlElement::Attribute(warn, "text")
                << "specified reduction=0, choosing reduction=1";

            reduction = 1;
        }

        foundReduction = true;
    }

    if (inbuf.getUnsigned("blocksize", blocksize)) {
        if (!blocksize) {
            XmlElement warn("warn", ostream, streamlock);
            XmlElement::Attribute(warn, "command") << "xsad";
            XmlElement::Attribute(warn, "text")
                << "specified blocksize=0, choosing blocksize=1";

            blocksize = 1;
        }

        foundBlocksize = true;
    }

    if (!inbuf.getUnsigned("precision", precision))
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
        subscriptionManager[main->getTask(ts)]->subscribe( channel[*it], event,
                    sync, reduction * mainSignal->decimation,
                    blocksize, base64, precision);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::xsod()
{
    std::list<unsigned int> intList;

    //cout << __LINE__ << "xsod: " << endl;

    if (inbuf.getUnsignedList("channels", intList)) {
        const VariableDirectory::Channels& channel = root.getChannels();
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

    Socket::state = CONNECTED;
    inBytes = 0;
    outBytes = 0;
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
