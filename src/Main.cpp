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

#include "config.h"
#include "Debug.h"

#include <cerrno>
#include <unistd.h>     // fork(), getpid(), chdir, sysconf, close
#include <signal.h>     // signal()
#include <limits.h>     // _POSIX_OPEN_MAX
#include <sys/types.h>  // umask()
#include <sys/stat.h>   // umask()

#include <log4cplus/syslogappender.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/streams.h>
#include <log4cplus/configurator.h>
#include <log4cplus/loggingmacros.h>

#include "Main.h"
#include "Task.h"
#include "Signal.h"
#include "ProcessParameter.h"
#include "Database.h"
#include "Config.h"
//#include "etlproto/Server.h"
#include "msrproto/Server.h"
#include "supervisor/Server.h"

using namespace PdServ;

/////////////////////////////////////////////////////////////////////////////
Main::Main(const std::string& name, const std::string& version):
    name(name), version(version), mutex(1),
    parameterLog(log4cplus::Logger::getInstance(
                LOG4CPLUS_TEXT("parameter")))
{
    msrproto = 0;
    supervisor = 0;
}

/////////////////////////////////////////////////////////////////////////////
Main::~Main()
{
}

/////////////////////////////////////////////////////////////////////////////
void Main::setupLogging()
{
#ifdef PDS_DEBUG
    consoleLogging();
#endif

    if (config("logging")) {
        typedef std::basic_istringstream<log4cplus::tchar> tistringstream;
        tistringstream is(
                LOG4CPLUS_STRING_TO_TSTRING(config("logging").toString()));
        log4cplus::PropertyConfigurator(is).configure();
    }
    else {
        syslogLogging();
    }

    LOG4CPLUS_INFO(log4cplus::Logger::getRoot(),
            LOG4CPLUS_TEXT("Started application ")
            << LOG4CPLUS_STRING_TO_TSTRING(name)
            << LOG4CPLUS_TEXT(", Version ")
            << LOG4CPLUS_STRING_TO_TSTRING(version));
}

/////////////////////////////////////////////////////////////////////////////
void Main::consoleLogging()
{
    log4cplus::BasicConfigurator::doConfigure();
//    log4cplus::SharedAppenderPtr cerr(new log4cplus::ConsoleAppender(true));
//    cerr->setLayout(
//        std::auto_ptr<log4cplus::Layout>(new log4cplus::PatternLayout(
//                LOG4CPLUS_TEXT("%D %p %c %x: %m"))));
//
//    log4cplus::Logger::getRoot().addAppender(cerr);
}

/////////////////////////////////////////////////////////////////////////////
void Main::syslogLogging()
{
    log4cplus::helpers::Properties p;
    p.setProperty(LOG4CPLUS_TEXT("ident"),
            LOG4CPLUS_STRING_TO_TSTRING(name));
    p.setProperty(LOG4CPLUS_TEXT("facility"),LOG4CPLUS_TEXT("local0"));

    log4cplus::SharedAppenderPtr appender( new log4cplus::SysLogAppender(p));
    appender->setLayout(
            std::auto_ptr<log4cplus::Layout>(new log4cplus::PatternLayout(
                    LOG4CPLUS_TEXT("%-5p %c <%x>: %m"))));

    log4cplus::Logger root = log4cplus::Logger::getRoot();
    root.addAppender(appender);
    root.setLogLevel(log4cplus::INFO_LOG_LEVEL);
}

/////////////////////////////////////////////////////////////////////////////
int Main::gettime(struct timespec* t) const
{
    return localtime(t);
}

/////////////////////////////////////////////////////////////////////////////
int Main::localtime(struct timespec* t)
{
    struct timeval tv;

    if (gettimeofday(&tv, 0))
        return errno;
    t->tv_sec = tv.tv_sec;
    t->tv_nsec = tv.tv_usec * 1000;

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
void Main::getSessionStatistics(std::list<SessionStatistics>& stats) const
{
    msrproto->getSessionStatistics(stats);
    //etlproto->getSessionStatistics(stats);
}

/////////////////////////////////////////////////////////////////////////////
void Main::startServers()
{
    LOG4CPLUS_INFO_STR(log4cplus::Logger::getRoot(),
            LOG4CPLUS_TEXT("Starting servers"));

    supervisor = new Supervisor::Server(this, config("supervisor"));
    msrproto   = new   MsrProto::Server(this, config("msr"));

//    EtlProto::Server etlproto(this);
}

/////////////////////////////////////////////////////////////////////////////
void Main::stopServers()
{
    delete msrproto;
    delete supervisor;

    savePersistent();

    LOG4CPLUS_INFO_STR(log4cplus::Logger::getRoot(),
            LOG4CPLUS_TEXT("Shut down servers"));
}

/////////////////////////////////////////////////////////////////////////////
int Main::setValue(const ProcessParameter* p, const Session* /*session*/,
        const char* buf, size_t offset, size_t count)
{
    // Ask the implemented parameter to change value. This allows the 
    // parameter to obtain a write lock
    int rv = p->setValue(buf, offset, count);
    if (rv)
        return rv;

    msrproto->parameterChanged(p, offset, count);

    PersistentMap::iterator it = persistentMap.find(p);
    bool persistent = it != persistentMap.end();
    bool log = parameterLog.isEnabledFor(log4cplus::INFO_LOG_LEVEL);
    std::string logString;

    // Setup logString when parameter has to be logged
    if ((persistent and persistentLogTraceOn) or log) {
        std::ostringstream os;
        os.imbue(std::locale::classic());
        os << p->path;

        os << " = ";

        static_cast<const ProcessParameter*>(p)->print(os, 0, p->memSize);

        logString = os.str();
    }

    // Save persistent parameter
    if (persistent) {
        char data[p->memSize];
        struct timespec time;

        static_cast<const Parameter*>(p)->getValue(0, data, &time);

        PdServ::Database(persistentLog,
                persistentConfig["database"].toString())
            .save(p, data, &time);

        if (persistentLogTraceOn)
            LOG4CPLUS_INFO_STR(persistentLogTrace,
                    LOG4CPLUS_STRING_TO_TSTRING(logString));

        if (it->second) {
            LOG4CPLUS_WARN(persistentLog,
                    LOG4CPLUS_TEXT("Persistent parameter ")
                    << LOG4CPLUS_STRING_TO_TSTRING(p->path)
                    << LOG4CPLUS_TEXT(" is coupled to signal ")
                    << LOG4CPLUS_STRING_TO_TSTRING(it->second->path)
                    << LOG4CPLUS_TEXT(". Manually setting a parameter-signal"
                        " pair removes this coupling"));
            it->second = 0;
        }
    }

    // Log parameter change
    if (log) {
        parameterLog.forcedLog(log4cplus::INFO_LOG_LEVEL,
                LOG4CPLUS_STRING_TO_TSTRING(logString));
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
void Main::poll(const Session *session, const Signal * const *s,
        size_t nelem, void *buf, struct timespec *t) const
{
    ost::SemaphoreLock lock(mutex);
    char *dest = reinterpret_cast<char*>(buf);
    double delay = 0.01;
    void *pollDest[nelem];

    for (size_t i = 0; i < nelem; ++i) {
        delay = std::max(delay,
                std::min(0.1, s[i]->poll(session, dest, t)));
        pollDest[i] = dest;
        dest += s[i]->memSize;
    }

    processPoll(static_cast<unsigned>(delay * 1000 + 0.5),
		    s, nelem, pollDest, t);
}

/////////////////////////////////////////////////////////////////////////////
unsigned int Main::setupPersistent()
{
    std::set<std::string> keys;

    persistentConfig = config("persistent");
    if (!persistentConfig)
        return 0;

    // Get variable list
    PdServ::Config varList(persistentConfig["variables"]);
    if (!varList[size_t(0)]) {
        LOG4CPLUS_INFO_STR(persistentLog,
                LOG4CPLUS_TEXT("Persistent variable list is empty"));
        return 0;
    }

    persistentLog = log4cplus::Logger::getRoot();
    persistentLogTraceOn = persistentConfig["trace"].toUInt();
    if (persistentLogTraceOn)
        persistentLogTrace =
            log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("persistent"));

    const std::string databasePath(persistentConfig["database"].toString());
    if (databasePath.empty()) {
        LOG4CPLUS_WARN_STR(persistentLog,
                LOG4CPLUS_TEXT("No persistent database path spedified"));
        return 0;
    }

    Database dataBase(persistentLog, databasePath);

    // Copy parameters
    typedef std::map<std::string, const Parameter*> ParameterMap;
    ParameterMap parameterMap;

    typedef std::list<const Parameter*> ParameterList;
    ParameterList parameters = getParameters();
    for (ParameterList::iterator it = parameters.begin();
            it != parameters.end(); ++it)
        parameterMap[(*it)->path] = *it;

    // Copy signals from tasks
    typedef std::map<std::string, const Signal*> SignalMap;
    SignalMap signalMap;

    typedef std::list<const Task*> TaskList;
    TaskList task = getTasks();
    for (TaskList::iterator it = task.begin();
            it != task.end(); ++it) {
        typedef std::list<const PdServ::Signal*> SignalList;

        SignalList signals =
            static_cast<const PdServ::Task*>(*it)->getSignals();

        for (SignalList::iterator it = signals.begin();
                it != signals.end(); ++it)
            signalMap[(*it)->path] = *it;
    }


    // Go through the variables section of the configuration to
    // find parameters to watch. There are 2 ways of specifying a variable
    // 1) string only pointing to a parameter to watch
    // 2) map with "parameter:" string pointing to a parameter to watch
    // 2) map with "parameter" and "signal" of a pair to watch
    // e.g.
    // variables:
    //     - "/path/to/parameter"
    //     - parameter: "/path/to/parameter"
    //     - parameter: /path/to/integrator/offset
    //       signal: "/path/to/integrator"
    for (size_t i = 0; varList[i]; ++i) {
        const PdServ::Config& item = varList[i];
        const Parameter* param;
        const Signal* signal = 0;

        std::string path =
            (item.isMapping() ? item["parameter"] : item).toString();

        ParameterMap::const_iterator paramIt(parameterMap.find(path));
        if (paramIt == parameterMap.end()) {
            // Parameter does not exist
            LOG4CPLUS_WARN(persistentLog,
                    LOG4CPLUS_TEXT("Persistent parameter ")
                    << LOG4CPLUS_STRING_TO_TSTRING(path)
                    << LOG4CPLUS_TEXT(" does not exist."));
            continue;
        }
        param = paramIt->second;

        if (persistentMap.find(param) != persistentMap.end()) {
            LOG4CPLUS_INFO(persistentLog,
                    LOG4CPLUS_TEXT("Persistent parameter ")
                    << LOG4CPLUS_STRING_TO_TSTRING(path)
                    << LOG4CPLUS_TEXT(" traced already."));
            continue;
        }

        if (item.isMapping() and item["signal"]) {
            path = item["signal"].toString();

            if (!path.empty()) {
                // Parameter <-> Signal pair
                SignalMap::const_iterator srcIt(signalMap.find(path));

                if (srcIt == signalMap.end()) {
                    // Signal does not exist
                    LOG4CPLUS_WARN(persistentLog,
                            LOG4CPLUS_TEXT("Signal ")
                            << LOG4CPLUS_STRING_TO_TSTRING(path)
                            << LOG4CPLUS_TEXT(
                                " of persistent pair does not exist."));
                    continue;
                }
                signal = srcIt->second;
            }
        }

        if (signal) {
            // Check whether signal is compatable to parameter
            if (signal->dtype != param->dtype) {
                // Data types do not match
                LOG4CPLUS_WARN(persistentLog,
                        LOG4CPLUS_TEXT("Data type of signal ")
                        << LOG4CPLUS_STRING_TO_TSTRING(signal->path)
                        << LOG4CPLUS_TEXT(" and parameter ")
                        << LOG4CPLUS_STRING_TO_TSTRING(param->path)
                        << LOG4CPLUS_TEXT(" does not match"));
                continue;
            }
            else if (signal->dim != param->dim) {
                // Data dimensions do not match
                LOG4CPLUS_WARN(persistentLog,
                        LOG4CPLUS_TEXT("Data dimension of signal ")
                        << LOG4CPLUS_STRING_TO_TSTRING(signal->path)
                        << LOG4CPLUS_TEXT(" and parameter ")
                        << LOG4CPLUS_STRING_TO_TSTRING(param->path)
                        << LOG4CPLUS_TEXT(" does not match"));
                continue;
            }
        }

        persistentMap[param] = signal;
        keys.insert(param->path);
        if (signal)
            LOG4CPLUS_DEBUG(persistentLog,
                    LOG4CPLUS_TEXT("Added persistent parameter-signal pair: ")
                    << LOG4CPLUS_STRING_TO_TSTRING(signal->path)
                    << LOG4CPLUS_TEXT(" -> ")
                    << LOG4CPLUS_STRING_TO_TSTRING(param->path));
        else
            LOG4CPLUS_DEBUG(persistentLog,
                    LOG4CPLUS_TEXT("Added persistent parameter: ")
                    << LOG4CPLUS_STRING_TO_TSTRING(param->path));

        // Last but not least, read peristent value from database and
        // set parameter
        const struct timespec* mtime;
        const char* value;
        if (dataBase.read(param, &value, &mtime)) {
            initializeParameter(param, value, mtime, signal);
            if (persistentLogTraceOn) {
                std::ostringstream os;
                os.imbue(std::locale::classic());
                os << param->path;

                os << " = ";

                signal->dtype.print(os, value, value, value + signal->memSize);

                LOG4CPLUS_INFO_STR(persistentLogTrace,
                        LOG4CPLUS_STRING_TO_TSTRING(os.str()));
            }
        }
        else if (signal)
            initializeParameter(param, 0, 0, signal);
    }

    // Purge unneeded parameters from database, unless disabled by
    // configuration
    Config cleanup(persistentConfig["cleanup"]);
    if (!cleanup or cleanup.toUInt())
        dataBase.checkKeys(keys);

    return persistentConfig["interval"].toUInt();
}

/////////////////////////////////////////////////////////////////////////////
void Main::savePersistent()
{
    if (persistentMap.empty())
        return;

    LOG4CPLUS_INFO_STR(persistentLog,
            LOG4CPLUS_TEXT("Saving persistent parameters"));

    // Open database and return if there are problems
    PdServ::Database db(persistentLog,
            persistentConfig["database"].toString());
    if (!db)
        return;

    // Only save signal/parameter pairs. Persistent paremters are saved as
    // they are changed
    for (PersistentMap::iterator it = persistentMap.begin();
            it != persistentMap.end(); ++it) {
        const Signal* signal = it->second;
        if (!signal)
            continue;

        const Parameter* param = it->first;
        char buf[param->memSize];
        struct timespec time;

        log_debug("Save %s", signal->path.c_str());
        if (getPersistentSignalValue(signal, buf, &time)) {
            db.save(param, buf, &time);

            if (persistentLogTraceOn) {
                std::ostringstream os;
                os.imbue(std::locale::classic());
                os << param->path;

                os << " = ";

                signal->dtype.print(os, buf, buf, buf + signal->memSize);

                LOG4CPLUS_INFO_STR(persistentLogTrace,
                        LOG4CPLUS_STRING_TO_TSTRING(os.str()));
            }

            LOG4CPLUS_TRACE(persistentLog,
                    LOG4CPLUS_TEXT("Saved persistent parameter: ")
                    << LOG4CPLUS_STRING_TO_TSTRING(param->path));
        }
    }
}
