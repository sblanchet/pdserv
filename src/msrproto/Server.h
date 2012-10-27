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

#include "../SessionStatistics.h"
#include "../Config.h"
#include "Parameter.h"
#include "Channel.h"

namespace log4cpp {
    class Category;
}

namespace PdServ {
    class Main;
    class Parameter;
}

namespace MsrProto {

class Session;

class Server: public DirectoryNode, public ost::Thread {
    public:
        Server(const PdServ::Main *main, const PdServ::Config& defaultConfig,
                const PdServ::Config &config);
        ~Server();

        void broadcast(Session *s, const std::string&);

        void setAic(const Parameter*);
        void parameterChanged(const PdServ::Parameter*,
                size_t startIndex, size_t n);

        void sessionClosed(Session *s);

        void getSessionStatistics(
                std::list<PdServ::SessionStatistics>& stats) const;

        const PdServ::Main * const main;
        log4cpp::Category &log;

        typedef std::vector<const Channel*> Channels;
        typedef std::vector<const Parameter*> Parameters;

        const Channels& getChannels() const;
        const Channel * getChannel(size_t) const;

        const Parameters& getParameters() const;
        const Parameter * getParameter(size_t) const;
        const Parameter * find(const PdServ::Parameter *p) const;

        template <typename T>
            const T * find(const std::string& path) const;

    private:
        std::set<Session*> sessions;
        int port;
        bool traditional;

        Channels channels;
        Parameters parameters;
        typedef std::map<const PdServ::Parameter *, const Parameter *>
            ParameterMap;
        ParameterMap parameterMap;

        mutable ost::Semaphore mutex;

        // Reimplemented from ost::Thread
        void initial();
        void run();
        void final();

        void createChildren(Variable* var);
        DirectoryNode* createChild(Variable* var,
                DirectoryNode *dir, const std::string& name,
                const PdServ::DataType& dtype,
                size_t nelem, size_t offset);
        size_t createChildren(Variable* var, DirectoryNode* dir,
                const PdServ::DataType& dtype,
                const PdServ::DataType::DimType& dim, size_t dimIdx,
                size_t offset);
        size_t createCompoundChildren(Variable* var, DirectoryNode* dir,
                const PdServ::DataType& dtype,
                const PdServ::DataType::DimType& dim, size_t dimIdx,
                size_t offset);
        size_t createVectorChildren(Variable* var,
                DirectoryNode* dir, const std::string& name,
                const PdServ::DataType& dtype,
                const PdServ::DataType::DimType& dim, size_t dimIdx,
                size_t offset);

};

/////////////////////////////////////////////////////////////////////////////
template <typename T>
const T *Server::find(const std::string &p) const
{
    if (p.empty() or p[0] != '/')
        return 0;

    const DirectoryNode *node = DirectoryNode::find(p, 1);
    return node ? dynamic_cast<const T*>(node) : 0;
}

}
#endif //MSRSERVER_H
