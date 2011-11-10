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

#include "config.h"

#include "../Main.h"
#include "../Task.h"
#include "../Signal.h"
#include "../Parameter.h"
#include "../SessionTaskData.h"
#include "../TaskStatistics.h"

#include "Session.h"
#include "PrintVariable.h"
#include "Server.h"
#include "Channel.h"
#include "Parameter.h"
#include "Directory.h"
#include "SubscriptionManager.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Session::Session( Server *s, ost::SocketService *ss,
        ost::TCPSocket &socket, const PdServ::Main *main):
    SocketPort(0, socket), PdServ::Session(main),
    server(s), dataTag("data"), outbuf(this)
{
//    cout << __LINE__ << __PRETTY_FUNCTION__ << this << endl;

    for (size_t i = 0; i < main->numTasks(); ++i) {
        double ts = main->getTask(i)->sampleTime;

        subscriptionManager[ts] = new SubscriptionManager(this);
        if (!i or ts < primaryTaskSampleTime)
            primaryTaskSampleTime = ts;
    }

    // Setup some internal variables
    writeAccess = false;
    echoOn = false;
    quiet = false;

    // Non-blocking read() and write()
    setCompletion(false);

    attach(ss);

    // Greet the new client
    MsrXml::Element greeting("connected");
    greeting.setAttribute("name", "MSR");
    greeting.setAttribute("host", "localhost"); // FIXME
    greeting.setAttribute("version", MSR_VERSION);
    greeting.setAttribute("features", MSR_FEATURES);
    greeting.setAttribute("recievebufsize", 100000000);
    outbuf << greeting << std::flush;

    expired();
}

/////////////////////////////////////////////////////////////////////////////
Session::~Session()
{
//    cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    for (SubscriptionManagerMap::iterator it = subscriptionManager.begin();
            it != subscriptionManager.end(); ++it)
        delete it->second;

    server->sessionClosed(this);
}

/////////////////////////////////////////////////////////////////////////////
void Session::broadcast(Session *s, const MsrXml::Element &element)
{
    outbuf << element << std::flush;
}

/////////////////////////////////////////////////////////////////////////////
void Session::parameterChanged(const PdServ::Parameter *p,
        size_t startIndex, size_t nelem)
{
    MsrXml::Element pu("pu");
    std::set<size_t> indices =
        server->getRoot().getParameterIndex(p, startIndex, nelem);

    for (std::set<size_t>::iterator it = indices.begin();
            it != indices.end(); ++it) {
        pu.setAttribute("index", *it);
        outbuf << pu;
    }

    outbuf << std::flush;
}

/////////////////////////////////////////////////////////////////////////////
void Session::requestOutput()
{
    setDetectOutput(true);
}

/////////////////////////////////////////////////////////////////////////////
void Session::expired()
{
    if (getDetectPending()) {
        // Read and process the incoming data
        if (rxPdo()) {
            // Unknown command warning
            MsrXml::Element error("error");
            error.setAttribute("text", "process synchronization lost");

            outbuf << error << std::flush;
        }
        else
            setTimer(100);      // Wakeup in 100ms again
    }
    else /*if (!getDetectPending())*/ {
        delete this;
        return;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::pending()
{
//    cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    int n, count;
    size_t _inBytes = inBytes;

    do {
        count = inbuf.free();
        n = receive(inbuf.bufptr(), count);

//        cout << std::string(inbuf.bufptr(), n);

        // No more data available or there was an error
        if (n <= 0)
            break;

        inBytes += n;   // PdServ::Session input byte counter

        if (inbuf.newData(n)) {
            do {
                processCommand();
            } while (inbuf.next());
        }

    } while (n == count);

    if (_inBytes == inBytes) {
        // End of stream
        setDetectPending(false);
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
        setDetectOutput(false);
        return;
    }
//    cout << std::string(outbuf.bufptr(), n);

    outBytes += n;       // PdServ::Session output byte counter

    if (outbuf.clear(n))
        setDetectOutput(false);
}

/////////////////////////////////////////////////////////////////////////////
void Session::disconnect()
{
//    cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    delete this;
}

/////////////////////////////////////////////////////////////////////////////
void Session::newSignalList(const PdServ::Task *task,
        const PdServ::Signal * const *s, size_t n)
{
    if (!subscriptionManager[task->sampleTime]->newSignalList(s, n))
        return;

    for (SubscriptionManagerMap::iterator it = subscriptionManager.begin();
            it != subscriptionManager.end(); ++it)
        it->second->sync();
}

/////////////////////////////////////////////////////////////////////////////
void Session::newSignalData(const PdServ::SessionTaskData *std)
{
    if (quiet)
        return;

    const struct timespec& t = std->taskStatistics->time;

    dataTag.setAttribute("level", std->task->sampleTime);
    dataTag.setAttribute("time", t);

    double ts = std->task->sampleTime;
    if (primaryTaskSampleTime == ts) {
        primaryTaskTime = 1.0e-9 * t.tv_nsec + t.tv_sec;
        cout << "time =" << primaryTaskTime << endl;
    }

    subscriptionManager[ts]->newSignalData(&dataTag, std);

    if (dataTag.hasChildren())
        outbuf << dataTag << std::flush;

    dataTag.releaseChildren();
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
                MsrXml::Element ack("ack");
                ack.setAttributeCheck("id", id);
                outbuf << ack << std::flush;
            }

            // Finished
            return;
        }
    }

    // Unknown command warning
    MsrXml::Element warn("warn");
    warn.setAttribute("num", 1000);
    warn.setAttribute("text", "unknown command");
    warn.setAttributeCheck("command", command);
    outbuf << warn << std::flush;
}

/////////////////////////////////////////////////////////////////////////////
void Session::broadcast()
{
    MsrXml::Element broadcast("broadcast");
    struct timespec ts;
    std::string s;

    main->gettime(&ts);

    broadcast.setAttribute("time", ts);

    if (inbuf.getString("action", s)) {
        broadcast.setAttributeCheck("action", s);
    }

    if (inbuf.getString("text",s)) {
        broadcast.setAttributeCheck("text", s);
    }
    server->broadcast(this, broadcast);
}

/////////////////////////////////////////////////////////////////////////////
void Session::echo()
{
    echoOn = inbuf.isTrue("value");
}

/////////////////////////////////////////////////////////////////////////////
void Session::ping()
{
    MsrXml::Element ping("ping");
    std::string id;

    if (inbuf.getString("id",id))
        ping.setAttributeCheck("id", id);

    outbuf << ping << std::flush;
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
    else {
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

        MsrXml::Element channels("channels");
        for (VariableDirectory::Channels::const_iterator it = chanList.begin();
                it != chanList.end(); it++) {
            MsrXml::Element *el = channels.createChild("channel");
            (*it)->setXmlAttributes(el, shortReply,
                    buf + bufOffset[(*it)->signal]);
        }

        outbuf << channels << std::flush;
        return;
    }

    // A single signal was requested
    if (c) {
        char buf[c->signal->memSize];

        c->signal->getValue(this, buf);

        MsrXml::Element channel("channel");
        c->setXmlAttributes(&channel, shortReply, buf);

        outbuf << channel << std::flush;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::readParameter()
{
    const Parameter *p = 0;
    bool shortReply = inbuf.isTrue("short");
    bool hex = inbuf.isTrue("hex");
    std::string name;
    unsigned int index;

    if (inbuf.getString("name", name)) {
        p = server->getRoot().findParameter(name.c_str());
    }
    else if (inbuf.getUnsigned("index", index)) {
        p = server->getRoot().getParameter(index);
    }
    else {
        const VariableDirectory::Parameters& parameter =
            server->getRoot().getParameters();
        MsrXml::Element parameters("parameters");

        for (VariableDirectory::Parameters::const_iterator it =
                parameter.begin(); it != parameter.end(); it++)
            (*it)->setXmlAttributes(this, parameters.createChild("parameter"),
                    shortReply, hex, writeAccess);

        outbuf << parameters << std::flush;
        return;
    }
    
    if (p) {
        MsrXml::Element parameter("parameter");
        p->setXmlAttributes(this, &parameter, shortReply, hex, writeAccess);
        outbuf << parameter << std::flush;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::readParamValues()
{
    MsrXml::Element param_values("param_values");
    const VariableDirectory::Parameters& parameter =
        server->getRoot().getParameters();
    std::string v;

    for (VariableDirectory::Parameters::const_iterator it = parameter.begin();
            it != parameter.end(); it++) {
        if (it != parameter.begin()) v.append(1,'|');

        char buf[(*it)->memSize];
        (*it)->getValue(this, buf);
        v.append(toCSV((*it)->printFunc, (*it)->mainParam, (*it)->nelem, buf));
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
    typedef std::list<PdServ::SessionStatistics> StatList;
    StatList stats;
    main->getSessionStatistics(stats);

    MsrXml::Element clients("clients");
    for (StatList::const_iterator it = stats.begin();
            it != stats.end(); it++) {
        MsrXml::Element *e = clients.createChild("client");
        e->setAttributeCheck("name",
                (*it).remote.size() ? (*it).remote : "unknown");
        e->setAttributeCheck("apname",
                (*it).client.size() ? (*it).client : "unknown");
        e->setAttribute("countin", (*it).countIn);
        e->setAttribute("countout", (*it).countOut);
        e->setAttribute("connectedtime", (*it).connectedTime);
    }

    outbuf << clients << std::flush;
}

/////////////////////////////////////////////////////////////////////////////
void Session::remoteHost()
{
    inbuf.getString("name", PdServ::Session::remoteHost);

    inbuf.getString("applicationname", client);

    writeAccess = inbuf.isEqual("access", "allow");

    if (writeAccess and inbuf.isTrue("isadmin")) {
        struct timespec ts;
        std::ostringstream os;
        main->gettime(&ts);

        os << "Adminmode filp: " << so; // 'so' is the fd and comes from
                                        // ost::Socket
        MsrXml::Element info("info");
        info.setAttribute("time", ts);
        info.setAttribute("text", os.str());
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
        errno = p->setHexValue(s, startindex, count);
    }
    else if (inbuf.find("value", s)) {
        errno = p->setDoubleValue(s, startindex, count);
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
    unsigned int reduction, blocksize, precision = 10;
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
            MsrXml::Element warn("warn");
            warn.setAttribute("command", "xsad");
            warn.setAttribute("text",
                    "specified reduction=0, choosing reduction=1");
            outbuf << warn << std::flush;

            reduction = 1;
        }
        foundReduction = true;
    }

    if (inbuf.getUnsigned("blocksize", blocksize)) {
        if (!blocksize) {
            MsrXml::Element warn("warn");
            warn.setAttribute("command", "xsad");
            warn.setAttribute("text",
                    "specified blocksize=0, choosing blocksize=1");
            outbuf << warn << std::flush;

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
        subscriptionManager[ts]->subscribe( channel[*it], event,
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
                subscriptionManager[ts]->unsubscribe(channel[*it]);
            }
        }
    }
    else
        for (SubscriptionManagerMap::iterator it = subscriptionManager.begin();
                it != subscriptionManager.end(); ++it)
            it->second->clear();
}

/////////////////////////////////////////////////////////////////////////////
const double *Session::getDblTimePtr() const
{
    return &primaryTaskTime;
}
