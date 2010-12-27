/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdcomserv package.
 *
 *  pdcomserv is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pdcomserv is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pdcomserv. See COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include "config.h"

#include "../Main.h"
#include "../Task.h"
#include "../Parameter.h"
#include "../Signal.h"
#include "../Receiver.h"

#include "Session.h"
#include "PrintVariable.h"
#include "Server.h"
#include "Channel.h"
#include "Parameter.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

#define MSR_R   0x01    /* Parameter is readable */
#define MSR_W   0x02    /* Parameter is writeable */
#define MSR_WOP 0x04    /* Parameter is writeable in real-time */
#define MSR_MONOTONIC 0x8 /* List must be monotonic */
#define MSR_S   0x10    /* Parameter must be saved by clients */
#define MSR_G   0x20    /* Gruppenbezeichnung (unused) */
#define MSR_AW  0x40    /* Writeable by admin only */
#define MSR_P   0x80    /* Persistant parameter, written to disk */
#define MSR_DEP 0x100   /* This parameter is an exerpt of another parameter.
                           Writing to this parameter also causes an update
                           notice for the encompassing parameter */
#define MSR_AIC 0x200   /* Asynchronous input channel */

/* ein paar Kombinationen */
#define MSR_RWS (MSR_R | MSR_W | MSR_S)
#define MSR_RP (MSR_R | MSR_P)


using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Session::Session( Server *s, ost::SocketService *ss,
        ost::TCPSocket &socket, HRTLab::Main *main):
    SocketPort(0, socket), HRTLab::Session(main),
    server(s), subscriptionManager(this),
    dataTag("data"), outbuf(this), inbuf(this)
{
//    cout << __LINE__ << __PRETTY_FUNCTION__ << this << endl;

    // Setup some internal variables
    writeAccess = false;
    echoOn = false;
    quiet = false;
    requestState = 0;
    dataTag.setAttribute("level", 0);

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
    server->sessionClosed(this);
}

/////////////////////////////////////////////////////////////////////////////
void Session::broadcast(Session *s, const MsrXml::Element &element)
{
    outbuf << element << std::flush;
}

/////////////////////////////////////////////////////////////////////////////
void Session::parametersChanged(const HRTLab::Parameter * const *p, size_t n) 
{
    while (n--) {
        const Server::Parameters& parameters = server->getParameters(*p++);
        for (Server::Parameters::const_iterator it = parameters.begin();
                it != parameters.end(); it++) {
            MsrXml::Element pu("pu");
            pu.setAttribute("index", (*it)->index);
            outbuf << pu << std::flush;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::requestOutput()
{
    setDetectOutput(true);
}

/////////////////////////////////////////////////////////////////////////////
void Session::requestSignal(const HRTLab::Signal *s, bool state)
{
    requestState = 2;
    signalRequest[s] = state;
}

/////////////////////////////////////////////////////////////////////////////
void Session::processSignalRequest()
{
    size_t insIdx = 0;
    size_t delIdx = 0;
    const HRTLab::Signal *ins[signalRequest.size()];
    const HRTLab::Signal *del[signalRequest.size()];
    for (SignalRequest::const_iterator it = signalRequest.begin();
            it != signalRequest.end(); it++) {
        if (it->second) 
            ins[insIdx++] = it->first;
        else
            del[delIdx++] = it->first;
    }

    signalRequest.clear();

    main->unsubscribe(this, del, delIdx);
    main->subscribe(this, ins, insIdx);
}

/////////////////////////////////////////////////////////////////////////////
void Session::expired()
{
    if (getDetectPending()) {
        // Read and process the incoming data
        rxPdo();

        if (!quiet and dataTag.hasChildren())
            outbuf << dataTag << std::flush;

        dataTag.releaseChildren();

        if (!--requestState)
            processSignalRequest();
    }
    else /*if (!getDetectPending())*/ {
        delete this;
        return;
    }

    setTimer(100);      // Wakeup in 100ms again
}

/////////////////////////////////////////////////////////////////////////////
void Session::pending()
{
//    cout << __LINE__ << __PRETTY_FUNCTION__ << endl;
    int n, count;
    size_t inputLen = 0;

    do {
        count = inbuf.free();
        n = receive(inbuf.bufptr(), count);

//        cout << std::string(inbuf.bufptr(), n);

        // No more data available or there was an error
        if (n <= 0)
            break;

        inBytes += n;   // HRTLab::Session input byte counter
        inputLen += n;  // Local input counter

        inbuf.parse(n);

    } while (n == count);

    if (!inputLen) {
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

    outBytes += n;       // HRTLab::Session output byte counter

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
void Session::newSignalList(const HRTLab::Task *task,
        const HRTLab::Signal * const *s, size_t n)
{
    subscriptionManager.newSignalList(task, s, n);
}

/////////////////////////////////////////////////////////////////////////////
void Session::newSignalData(const HRTLab::Receiver &receiver, const char *data)
{
    if (quiet)
        return;

    dataTag.setAttribute("time", *receiver.time);

    subscriptionManager.newSignalData(&dataTag, receiver, data);
}

/////////////////////////////////////////////////////////////////////////////
void Session::processCommand(const char *command, const Attr &attr)
{
    size_t commandLen = strlen(command);

    static struct {
        size_t len;
        const char *name;
        void (Session::*func)(const Attr &attr);
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
        {14, "read_parameter",          &Session::readParameter },
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
            (this->*cmds[idx].func)(attr);

            // If "ack" attribute was set, send it back
            if (attr.id) {
                MsrXml::Element ack("ack");
                ack.setAttributeCheck("id", *attr.id);
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
void Session::broadcast(const Attr &attr)
{
    MsrXml::Element broadcast("broadcast");
    struct timespec ts;
    std::string s;

    main->gettime(&ts);

    broadcast.setAttribute("time", ts);

    if (attr.getString("action", s)) {
        broadcast.setAttributeCheck("action", s);
    }

    if (attr.getString("text",s)) {
        broadcast.setAttributeCheck("text", s);
    }
    server->broadcast(this, broadcast);
}

/////////////////////////////////////////////////////////////////////////////
void Session::echo(const Attr &attr)
{
    echoOn = attr.isTrue("value");
}

/////////////////////////////////////////////////////////////////////////////
void Session::ping(const Attr &attr)
{
    MsrXml::Element ping("ping");

    if (attr.id)
        ping.setAttributeCheck("id", *attr.id);

    outbuf << ping << std::flush;
}

/////////////////////////////////////////////////////////////////////////////
void Session::readChannel(const Attr &attr)
{
    const Channel *c = 0;
    bool shortReply = attr.isTrue("short");
    std::string path;
    unsigned int index;

    if (attr.getString("name", path)) {
        c = server->getChannel(path);
    }
    else if (attr.getUnsigned("index", index)) {
        c = server->getChannel(index);
    }
    else {
        size_t buflen = 0;
        const HRTLab::Signal *mainSignal = 0;
        const Server::Channels& channel = server->getChannels();
        std::map<const HRTLab::Signal*, size_t> bufOffset;

        typedef std::list<const HRTLab::Signal*> SignalList;
        SignalList orderedSignals[HRTLab::Variable::maxWidth + 1];

        for (size_t i = 0; i < channel.size(); i++) {
            mainSignal = channel[i]->signal;
            if (bufOffset.find(mainSignal) != bufOffset.end())
                continue;

            bufOffset[mainSignal] = 0;
            orderedSignals[mainSignal->width].push_back(mainSignal);
        }

        const HRTLab::Signal *signalList[bufOffset.size()];

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
        main->getValues(signalList, bufOffset.size(), buf);

        MsrXml::Element channels("channels");
        for (size_t i = 0; i < channel.size(); i++) {
            MsrXml::Element *el = channels.createChild("channel");
            c = channel[i];
            c->setXmlAttributes(el, shortReply,
                    buf + bufOffset[c->signal]);
        }

        outbuf << channels << std::flush;
        return;
    }

    // A single signal was requested
    if (c) {
        char buf[c->signal->memSize];

        c->signal->getValue(buf);

        MsrXml::Element channel("channel");
        c->setXmlAttributes(&channel, shortReply, buf);

        outbuf << channel << std::flush;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::readParameter(const Attr &attr)
{
    const Parameter *p = 0;
    bool shortReply = attr.isTrue("short");
    bool hex = attr.isTrue("hex");
    std::string name;
    unsigned int index;
    unsigned int flags = writeAccess
        ? MSR_R | MSR_W | MSR_WOP | MSR_MONOTONIC : MSR_R | MSR_MONOTONIC;

    if (attr.getString("name", name)) {
        p = server->getParameter(name);
    }
    else if (attr.getUnsigned("index", index)) {
        p = server->getParameter(index);
    }
    else {
        const Server::Parameters& parameter = server->getParameters();
        MsrXml::Element parameters("parameters");

        for (Server::Parameters::const_iterator it = parameter.begin();
                it != parameter.end(); it++)
            (*it)->setXmlAttributes( parameters.createChild("parameter"),
                    shortReply, hex, flags);

        outbuf << parameters << std::flush;
        return;
    }
    
    if (p) {
        MsrXml::Element parameter("parameter");
        p->setXmlAttributes( &parameter, shortReply, hex, flags);
        outbuf << parameter << std::flush;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::readParamValues(const Attr &attr)
{
    MsrXml::Element param_values("param_values");
    const Server::Parameters& parameter = server->getParameters();
    std::string v;

    for (Server::Parameters::const_iterator it = parameter.begin();
            it != parameter.end(); it++) {
        if (it != parameter.begin()) v.append(1,'|');

        char buf[(*it)->memSize];
        (*it)->getValue(buf);
        v.append(toCSV((*it)->printFunc, (*it)->mainParam, (*it)->nelem, buf));
    }

    param_values.setAttribute("value", v);

    outbuf << param_values << std::flush;
}

/////////////////////////////////////////////////////////////////////////////
void Session::readStatistics(const Attr &attr)
{
    // <clients>
    //   <client index="0" name="lansim"
    //           apname="Persistent Manager, Version: 0.3.1"
    //           countin="19908501" countout="27337577"
    //           connectedtime="1282151176.659208"/>
    //   <client index="1" .../>
    // </clients>
    typedef std::list<HRTLab::SessionStatistics> StatList;
    StatList stats;
    main->getSessionStatistics(stats);
    int index = 0;

    MsrXml::Element clients("clients");
    for (StatList::const_iterator it = stats.begin();
            it != stats.end(); it++) {
        MsrXml::Element *e = clients.createChild("client");
        e->setAttribute("index", index++);
        e->setAttributeCheck("name", (*it).remote);
        e->setAttributeCheck("apname", (*it).client);
        e->setAttribute("countin", (*it).countIn);
        e->setAttribute("countout", (*it).countOut);
        e->setAttribute("connectedtime", (*it).connectedTime);
    }

    outbuf << clients << std::flush;
}

/////////////////////////////////////////////////////////////////////////////
void Session::remoteHost(const Attr &attr)
{
    attr.getString("name", HRTLab::Session::remoteHost);

    attr.getString("applicationname", client);

    writeAccess = attr.isEqual("access", "allow");

    if (writeAccess and attr.isTrue("isadmin")) {
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
void Session::writeParameter(const Attr &attr)
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
    if (attr.getString("name", name)) {
        p = server->getParameter(name);
    }
    else if (attr.getUnsigned("index", index)) {
        p = server->getParameter(index);
    }

    if (!p)
        return;
    
    size_t startindex = 0;
    if (attr.getUnsigned("startindex", startindex)) {
        if (startindex >= p->nelem)
            return;
    }

    int errno;
    char *s;
    if (attr.find("hexvalue", s)) {
        errno = p->setHexValue(s, startindex);
    }
    else if (attr.find("value", s)) {
        errno = p->setDoubleValue(s, startindex);
    }
    else
        return;

    if (errno) {
        // If an error occurred, tell this client to reread the value
        parametersChanged(&p->mainParam, 1);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::xsad(const Attr &attr)
{
    unsigned int reduction, blocksize, precision = 10;
    bool base64 = attr.isEqual("coding", "Base64");
    bool event = attr.isTrue("event");
    bool sync = attr.isTrue("sync");
    bool foundReduction = false, foundBlocksize = false;
    std::list<unsigned int> indexList;
    const Server::Channels& channel = server->getChannels();

    // Quiet will stop all transmission of <data> tags until
    // sync is called
    quiet = !sync and attr.isTrue("quiet");

    if (!attr.getUnsignedList("channels", indexList)) {
        if (sync)
            subscriptionManager.sync();
        return;
    }

    if (attr.getUnsigned("reduction", reduction)) {
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

    if (attr.getUnsigned("blocksize", blocksize)) {
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

    if (!attr.getUnsigned("precision", precision)) {
        precision = 10;
    }

    for (std::list<unsigned int>::const_iterator it = indexList.begin();
            it != indexList.end(); it++) {
        if (*it >= channel.size())
            continue;

        const HRTLab::Signal *mainSignal = channel[*it]->signal;

        if (event) {
            if (!foundReduction)
                // If user did not supply a reduction, limit to a 
                // max of 10Hz automatically
                reduction = 0.1 / mainSignal->task->sampleTime
                    / mainSignal->decimation + 0.5;
        }
        else if (!foundReduction and !foundBlocksize) {
            // Quite possibly user input; choose reduction for 1Hz
            reduction = 1.0 / mainSignal->task->sampleTime
                / mainSignal->decimation + 0.5;
            blocksize = 1;
        }
        else if (foundReduction and !foundBlocksize) {
            // Choose blocksize so that a datum is sent at 10Hz
            blocksize = 0.1 / mainSignal->task->sampleTime
                    / mainSignal->decimation + 0.5;
        }
        else if (!foundReduction and foundBlocksize) {
            reduction = 1;
        }

        if (!reduction) reduction = 1;
        if (!blocksize) blocksize = 1;

        subscriptionManager.subscribe( channel[*it],
                event, sync, reduction, blocksize, base64, precision);
    }
}

/////////////////////////////////////////////////////////////////////////////
void Session::xsod(const Attr &attr)
{
    std::list<unsigned int> intList;

//    cout << __LINE__ << "xsod: " << endl;

    if (attr.getUnsignedList("channels", intList)) {
        const Server::Channels& channel = server->getChannels();
        for (std::list<unsigned int>::const_iterator it = intList.begin();
                it != intList.end(); it++) {
            if (*it < channel.size()) {
                subscriptionManager.unsubscribe(channel[*it]);
            }
        }
    }
    else {
        subscriptionManager.unsubscribe();
    }
}
