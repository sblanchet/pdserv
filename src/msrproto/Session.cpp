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

#include <cerrno>       // ENAMETOOLONG
#include <climits>      // HOST_NAME_MAX
#include <unistd.h>     // gethostname

#include "../Debug.h"

#include "../Main.h"
#include "../Task.h"
#include "../Signal.h"
#include "../Parameter.h"
#include "../SessionShadow.h"
#include "../SessionTaskData.h"
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
Session::Session( Server *server, ost::TCPSocket &socket):
    TCPSession(socket), PdServ::Session(server->main),
    server(server)
{
    for (unsigned int i = 0; i < main->numTasks(); ++i) {
        const PdServ::Task *task = main->getTask(i);

        subscriptionManager[task] = new SubscriptionManager(this);
    }

    // Setup some internal variables
    writeAccess = false;
    echoOn = false;
    quiet = false;

    // Non-blocking read() and write()
    //setCompletion(false);

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
    XmlElement greeting("connected", this);
    XmlElement::Attribute(greeting, "name")
        << main->getName() << ' ' << main->getVersion();
    XmlElement::Attribute(greeting, "host")
        << reinterpret_cast<const char*>(hostname);
    XmlElement::Attribute(greeting, "version") << MSR_VERSION;
    XmlElement::Attribute(greeting, "features") << MSR_FEATURES;
    XmlElement::Attribute(greeting, "recievebufsize") << 100000000;

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
void Session::broadcast(Session *s, const XmlElement &element)
{
    //outbuf << element << std::flush;
}

/////////////////////////////////////////////////////////////////////////////
void Session::parameterChanged(const PdServ::Parameter *p,
        size_t startIndex, size_t nelem)
{
    const Parameter *param = server->getRoot().getParameter(p);
    param->valueChanged(this, startIndex, nelem);
}

/////////////////////////////////////////////////////////////////////////////
void Session::run()
{
    ssize_t n;
    int count;

    while (true) {
        count = inbuf.free();
        n = readData(inbuf.bufptr(), count, 0, 100);

        if (n < 0) {
            switch (getErrorNumber()) {
                case ost::Socket::errTimeout:
                    break;

                default:
                    return;
            }
        }
        else if (n) {
            inBytes += n;
            if (inbuf.newData(n)) {
                do {
                    processCommand();
                } while (inbuf.next());
            }
        }
        else {
            return;
        }

        if (rxPdo()) {
            // Unknown command warning
            XmlElement error("error", this);
            XmlElement::Attribute(error, "text")
                << "process synchronization lost";
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::newSignalList(const PdServ::Task *task,
        const PdServ::Signal * const *s, size_t n)
{
    if (!subscriptionManager[task]->newSignalList(s, n))
        return;

    for (SubscriptionManagerMap::iterator it = subscriptionManager.begin();
            it != subscriptionManager.end(); ++it)
        it->second->sync();
}

/////////////////////////////////////////////////////////////////////////////
void Session::newSignalData(const PdServ::SessionTaskData *std)
{
    PrintQ printQueue;
    subscriptionManager[std->task]->newSignalData(printQueue, std);

    if (!printQueue.empty() and !quiet) {
        const PdServ::TaskStatistics *stat =
            std->session->getTaskStatistics(std->task);
        XmlElement dataTag("data", this);
        XmlElement::Attribute(dataTag, "level") << 0;
        XmlElement::Attribute(dataTag, "time") << stat->time;

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

            // Call the method
            (this->*cmds[idx].func)();

            // If "ack" attribute was set, send it back
            std::string id;
            if (inbuf.getString("id", id)) {
                XmlElement ack("ack", this);
                XmlElement::Attribute(ack,"id").setWithCare(id.c_str());
            }

            // Finished
            return;
        }
    }

    // Unknown command warning
    XmlElement warn("warn", this);
    XmlElement::Attribute(warn, "num") << 1000;
    XmlElement::Attribute(warn, "text") << "unknown command";
    XmlElement::Attribute(warn, "command").setWithCare(command);
}

/////////////////////////////////////////////////////////////////////////////
void Session::broadcast()
{
    std::ostringstream os;
    XmlElement broadcast("broadcast", &os);
    struct timespec ts;
    std::string s;

    main->gettime(&ts);
    XmlElement::Attribute(broadcast, "time") << ts;

    if (inbuf.getString("action", s))
        XmlElement::Attribute(broadcast, "action").setWithCare(s.c_str());

    if (inbuf.getString("text", s))
        XmlElement::Attribute(broadcast, "text").setWithCare(s.c_str());

    //server->broadcast(this, broadcast);
}

/////////////////////////////////////////////////////////////////////////////
void Session::echo()
{
    echoOn = inbuf.isTrue("value");
}

/////////////////////////////////////////////////////////////////////////////
void Session::ping()
{
    XmlElement ping("ping", this);
    std::string id;

    if (inbuf.getString("id",id))
        XmlElement::Attribute(ping, "id").setWithCare(id.c_str());
}

/////////////////////////////////////////////////////////////////////////////
void Session::readChannel()
{
    const Channel *c = 0;
    bool shortReply = inbuf.isTrue("short");
    std::string path;
    unsigned int index;

    if (inbuf.getString("name", path)) {
        c = server->getRoot().findChannel(path.c_str());
    }
    else if (inbuf.getUnsigned("index", index)) {
        c = server->getRoot().getChannel(index);
    }

    // A single signal was requested
    if (c) {
        char buf[c->signal->memSize];

        c->signal->getValue(this, buf);

        XmlElement channel("channel", this);
        c->setXmlAttributes(channel, shortReply, buf, 16);

        return;
    }

    size_t buflen = 0;
    const PdServ::Signal *mainSignal = 0;
    std::map<const PdServ::Signal*, size_t> bufOffset;

    const VariableDirectory::Channels& chanList =
        server->getRoot().getChannels();

    typedef std::list<const PdServ::Signal*> SignalList;
    SignalList orderedSignals[PdServ::Variable::maxWidth + 1];

    for (VariableDirectory::Channels::const_iterator it = chanList.begin();
            it != chanList.end(); it++) {
        mainSignal = (*it)->signal;
        if (bufOffset.find(mainSignal) != bufOffset.end())
            continue;

        bufOffset[mainSignal] = 0;
        orderedSignals[mainSignal->width].push_back(mainSignal);
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

    XmlElement channels("channels", this);
    for (VariableDirectory::Channels::const_iterator it = chanList.begin();
            it != chanList.end(); it++)
        XmlElement("channel", channels).setAttributes(*it,
                buf + bufOffset[(*it)->signal], shortReply, 16);
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
        p = server->getRoot().findParameter(name.c_str());
    }
    else if (inbuf.getUnsigned("index", index)) {
        p = server->getRoot().getParameter(index);
    }

    if (p) {
        char buf[p->mainParam->memSize];
        struct timespec ts;

        p->mainParam->getValue(this, buf, &ts);

        std::string id;
        inbuf.getString("id", id);
        XmlElement ("parameter", this)
            .setAttributes(p, buf, ts, shortReply, hex, writeAccess, 16, id);

        return;
    }

    XmlElement parametersElement("parameters", this);

    const VariableDirectory::Parameters& parameters =
        server->getRoot().getParameters();
    VariableDirectory::Parameters::const_iterator it = parameters.begin();
    while ( it != parameters.end()) {
        const PdServ::Parameter* mainParam = (*it)->mainParam;
        char buf[mainParam->memSize];
        struct timespec ts;

        mainParam->getValue(this, buf, &ts);

        while (it != parameters.end() and mainParam == (*it)->mainParam)
            XmlElement ("parameter",  parametersElement).setAttributes(
                    *it++, buf, ts, shortReply, hex, writeAccess, 16);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::readParamValues()
{
    XmlElement param_values("param_values", this);
    XmlElement::Attribute values(param_values, "value");

    const VariableDirectory::Parameters& parameters =
        server->getRoot().getParameters();
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

    XmlElement clients("clients", this);
    for (StatList::const_iterator it = stats.begin();
            it != stats.end(); it++) {
        XmlElement client("client", clients);
        XmlElement::Attribute(client,"name").setWithCare(
                (*it).remote.size() ? (*it).remote.c_str() : "unknown");
        XmlElement::Attribute(client,"apname").setWithCare(
                (*it).client.size() ? (*it).client.c_str() : "unknown");
        XmlElement::Attribute(client,"countin") << (*it).countIn;
        XmlElement::Attribute(client,"countout") << (*it).countOut;
        XmlElement::Attribute(client,"connectedtime") << (*it).connectedTime;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::remoteHost()
{
    inbuf.getString("name", PdServ::Session::remoteHost);

    inbuf.getString("applicationname", client);

    writeAccess = inbuf.isEqual("access", "allow");

    if (writeAccess and inbuf.isTrue("isadmin")) {
        struct timespec ts;
        std::ostringstream os, message;
        main->gettime(&ts);

        os << "Adminmode filp: " << so; // 'so' is the fd and comes from
                                        // ost::Socket
        {
            XmlElement info("info", &message);
            XmlElement::Attribute(info, "time") << ts;
            XmlElement::Attribute(info, "text") << os.str();
        }

//        server->broadcast(this, info);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::writeParameter()
{
    if (!writeAccess) {
        XmlElement warn("warn", this);
        XmlElement::Attribute(warn, "text") << "No write access";
        return;
    }

    const Parameter *p = 0;

    unsigned int index;
    std::string name;
    if (inbuf.getString("name", name)) {
        p = server->getRoot().findParameter(name.c_str());
    }
    else if (inbuf.getUnsigned("index", index)) {
        p = server->getRoot().getParameter(index);
    }

    if (!p)
        return;
    
    unsigned int startindex = 0;
    if (inbuf.getUnsigned("startindex", startindex)) {
        if (startindex >= p->nelem)
            return;
    }

    int errno;
    const char *s;
    size_t count;
    if (inbuf.find("hexvalue", s)) {
        errno = p->setHexValue(this, s, startindex, count);
    }
    else if (inbuf.find("value", s)) {
        errno = p->setDoubleValue(this, s, startindex, count);
    }
    else
        return;

    if (errno) {
        // If an error occurred, tell this client to reread the value
        parameterChanged(p->mainParam, startindex, count);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::xsad()
{
    unsigned int reduction, blocksize, precision = 16;
    bool base64 = inbuf.isEqual("coding", "Base64");
    bool event = inbuf.isTrue("event");
    bool sync = inbuf.isTrue("sync");
    bool foundReduction = false, foundBlocksize = false;
    std::list<unsigned int> indexList;
    const VariableDirectory::Channels& channel =
        server->getRoot().getChannels();

    // Quiet will stop all transmission of <data> tags until
    // sync is called
    quiet = !sync and inbuf.isTrue("quiet");

    if (!inbuf.getUnsignedList("channels", indexList)) {
        if (sync)
            for (SubscriptionManagerMap::iterator it =
                    subscriptionManager.begin();
                    it != subscriptionManager.end(); ++it)
                it->second->sync();
        return;
    }

    if (inbuf.getUnsigned("reduction", reduction)) {
        if (!reduction) {
            XmlElement warn("warn", this);
            XmlElement::Attribute(warn, "command") << "xsad";
            XmlElement::Attribute(warn, "text")
                << "specified reduction=0, choosing reduction=1";

            reduction = 1;
        }
        foundReduction = true;
    }

    if (inbuf.getUnsigned("blocksize", blocksize)) {
        if (!blocksize) {
            XmlElement warn("warn", this);
            XmlElement::Attribute(warn, "command") << "xsad";
            XmlElement::Attribute(warn, "text")
                << "specified blocksize=0, choosing blocksize=1";

            blocksize = 1;
        }

        foundBlocksize = true;
    }

    if (!inbuf.getUnsigned("precision", precision)) {
        precision = 10;
    }

    for (std::list<unsigned int>::const_iterator it = indexList.begin();
            it != indexList.end(); it++) {
        if (*it >= channel.size())
            continue;

        const PdServ::Signal *mainSignal = channel[*it]->signal;

        if (event) {
            if (!foundReduction)
                // If user did not supply a reduction, limit to a 
                // max of 10Hz automatically
                reduction = 0.1 / mainSignal->sampleTime + 0.5;
        }
        else if (!foundReduction and !foundBlocksize) {
            // Quite possibly user input; choose reduction for 1Hz
            reduction = 1.0 / mainSignal->sampleTime + 0.5;
            blocksize = 1;
        }
        else if (foundReduction and !foundBlocksize) {
            // Choose blocksize so that a datum is sent at 10Hz
            blocksize = 0.1 / mainSignal->sampleTime + 0.5;
        }
        else if (!foundReduction and foundBlocksize) {
            reduction = 1;
        }

        if (!reduction) reduction = 1;
        if (!blocksize) blocksize = 1;

        double ts = mainSignal->sampleTime;
        subscriptionManager[main->getTask(ts)]->subscribe( channel[*it], event,
                    sync, reduction, blocksize, base64, precision);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::xsod()
{
    std::list<unsigned int> intList;

    //cout << __LINE__ << "xsod: " << endl;

    if (inbuf.getUnsignedList("channels", intList)) {
        const VariableDirectory::Channels& channel =
            server->getRoot().getChannels();
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
