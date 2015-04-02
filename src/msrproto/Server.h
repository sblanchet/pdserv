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

#ifndef MSRSERVER_H
#define MSRSERVER_H

#include <set>
#include <list>
#include <cc++/thread.h>
#include <log4cplus/logger.h>

#include "../SessionStatistics.h"
#include "../Config.h"
#include "Parameter.h"
#include "Channel.h"

namespace PdServ {
    class Main;
    class Task;
    class Parameter;
}

namespace MsrProto {

class Session;
class Event;

class Server: public ost::Thread {
    public:
        Server(const PdServ::Main *main, const PdServ::Config &config);
        ~Server();

        void broadcast(Session *s, const struct timespec& ts,
                const std::string& action, const std::string& text);

        void setAic(const Parameter*);
        void parameterChanged(const PdServ::Parameter*,
                size_t startIndex, size_t n);

        void sessionClosed(Session *s);

        void getSessionStatistics(
                std::list<PdServ::SessionStatistics>& stats) const;

        const PdServ::Main * const main;
        const log4cplus::Logger log;
        const bool* const active;

        typedef std::vector<const Channel*> Channels;
        typedef std::vector<const Parameter*> Parameters;
        typedef std::vector<const Event*> Events;

        const Channels& getChannels() const;
        const Channel * getChannel(size_t) const;
        void listDir(PdServ::Session *,
                XmlElement& xml, const std::string& path) const;

        const Parameters& getParameters() const;
        const Parameter * getParameter(size_t) const;
        const Parameter * find(const PdServ::Parameter *p) const;

        const Events& getEvents() const;

        template <typename T>
            const T * find(const std::string& path) const;

    private:
        std::set<Session*> sessions;
        int port;
        bool itemize;   // Split multidimensional variables to scalars
        bool _active;

        DirectoryNode variableDirectory;
        DirectoryNode* insertRoot;

        Events events;

        Channels channels;
        Parameters parameters;

        typedef std::map<const PdServ::Parameter *, const Parameter*>
            ParameterMap;
        ParameterMap parameterMap;

        mutable ost::Semaphore mutex;

        // Reimplemented from ost::Thread
        void initial();
        void run();
        void final();

        void createChannels(DirectoryNode* baseDir, size_t taskNum);
        void createParameters(DirectoryNode* baseDir);
        void createEvents();

        struct CreateVariable {
            CreateVariable(Server* server, DirectoryNode* baseDir,
                    const PdServ::Variable* var);
            CreateVariable(const CreateVariable& other);

            void newDimension(
                    const PdServ::DataType& dtype,
                    const PdServ::DataType::DimType& dim,
                    size_t dimIdx, size_t elemIdx,
                    CreateVariable& c, size_t offset);
            void newField(const PdServ::DataType::Field *field,
                    CreateVariable& c, size_t offset);
            bool newVariable(
                    const PdServ::DataType& dtype,
                    const PdServ::DataType::DimType& dim,
                    size_t dimIdx, size_t elemIdx, size_t offset) const;
            virtual bool createVariable(
                    const PdServ::DataType& dtype,
                    const PdServ::DataType::DimType& dim,
                    size_t offset) const = 0;

            std::string path() const;

            std::string name;

            const CreateVariable* const parent;
            Server* const server;
            DirectoryNode* const baseDir;
            const PdServ::Variable* const var;
        };

        struct CreateChannel: CreateVariable {
            CreateChannel(Server* server, DirectoryNode* baseDir,
                    size_t taskIdx, const PdServ::Signal* s);

            bool createVariable(
                    const PdServ::DataType& dtype,
                    const PdServ::DataType::DimType& dim,
                    size_t offset) const;

            const size_t taskIdx;
        };

        struct CreateParameter: CreateVariable {
            CreateParameter(Server* server, DirectoryNode* baseDir,
                    const PdServ::Parameter* p);

            bool createVariable(
                    const PdServ::DataType& dtype,
                    const PdServ::DataType::DimType& dim,
                    size_t offset) const;

            mutable Parameter* parentParameter;
        };
};

/////////////////////////////////////////////////////////////////////////////
template <typename T>
const T *Server::find(const std::string &p) const
{
    if (p.empty() or p[0] != '/')
        return 0;

    const DirectoryNode *node = variableDirectory.find(p, 1);
    return node ? dynamic_cast<const T*>(node) : 0;
}

}
#endif //MSRSERVER_H
