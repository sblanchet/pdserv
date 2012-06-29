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

#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <string>
#include <map>
#include <vector>
#include "../DataType.h"

namespace PdServ {
    class Variable;
    class Signal;
    class Parameter;
    class Session;
}

namespace MsrProto {

class Server;
class Variable;
class Channel;
class Parameter;
class XmlElement;

class DirectoryNode {
    public:
        explicit DirectoryNode(const std::string& name,
                DirectoryNode *parent);
        virtual ~DirectoryNode();

        void list(PdServ::Session *,
                XmlElement& parent, const char *path) const;
        std::string path() const;
        bool hasChildren() const;

        DirectoryNode *find(const char *path) const;

        DirectoryNode *mkdir(const char *path, char &hide,
                std::string &name, bool traditional);

        void rename(DirectoryNode* parent,
                const std::string& name);

        void insert(Variable *var, size_t index, size_t nelem,
                const PdServ::DataType::DimType& dim,
                size_t dimIdx = 0);

        void dump() const;

    protected:
        explicit DirectoryNode();
        DirectoryNode *mkdir(const std::string& path, std::string& name);

    private:
        std::string split(const char *&path,
                char &hide, bool traditional) const;

        typedef std::map<std::string, DirectoryNode*> Children;
        Children children;

        DirectoryNode * parent;
        const std::string* name;

        const std::string* insert(DirectoryNode* child,
                const std::string& name);
};

/////////////////////////////////////////////////////////////////////////////
class VariableDirectory: private DirectoryNode {
    public:
        VariableDirectory();
        ~VariableDirectory();

        void insert(const PdServ::Signal *signal,
                std::string prefix, bool traditional);
        void insert(const PdServ::Parameter *parameter,
                std::string prefix, bool traditional);

        void list(PdServ::Session *,
                XmlElement& parent, const char *path) const;

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
        Channels channels;
        Parameters parameters;

        typedef std::map<const PdServ::Parameter *, const Parameter *>
            ParameterMap;
        ParameterMap parameterMap;

        DirectoryNode *mkdir(std::string& path, char &hide, bool traditional);
        void insertChildren(const Channel* c);
        void insertChildren(const Parameter* p);
};

/////////////////////////////////////////////////////////////////////////////
template <typename T>
const T *VariableDirectory::find(const std::string &p) const
{
    if (p.empty() or p[0] != '/')
        return 0;

    DirectoryNode *node = DirectoryNode::find(p.c_str() + 1);
    return node ? dynamic_cast<const T*>(node) : 0;
}

}

#endif  // DIRECTORY_H
