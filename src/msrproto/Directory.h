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

#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <string>
#include <map>
#include <vector>
#include <set>

namespace PdServ {
    class Variable;
    class Signal;
    class Parameter;
}

namespace MsrProto {

class Variable;
class Channel;
class Parameter;

/////////////////////////////////////////////////////////////////////////////
class DirectoryNode {
    public:
        DirectoryNode(DirectoryNode *parent,
                const std::string& name, bool hide);
//        ~DirectoryNode();

        std::string path() const;

        const DirectoryNode *parent;
        const std::string name;
        const bool hidden;

        const Channel *findChannel(const std::string &path) const;
        const Parameter *findParameter(const std::string &path) const;
        const Variable *findVariable(const std::string &path,
                size_t pathOffset) const;

        DirectoryNode *mkdir(const PdServ::Variable *v, size_t index,
                size_t elementCount, unsigned int pathOffset = 0);
        DirectoryNode *mkdir(size_t index,
                size_t nelem, size_t ndims, const size_t *dim);

        void insert(const Variable *v);

    protected:
        DirectoryNode();

        const Variable *variable;

    private:
        typedef std::map<const std::string, DirectoryNode*> Entry;
        Entry entry;
};

/////////////////////////////////////////////////////////////////////////////
class VariableDirectory: public DirectoryNode {
    public:
        VariableDirectory();
        ~VariableDirectory();

        bool insert(const PdServ::Signal *s, size_t index = 0,
                size_t elementCount = 0);
        bool insert(const PdServ::Parameter *p, size_t index = 0,
                size_t elementCount = 0, bool dependent = false);

       typedef std::vector<const Channel*> Channels;
       typedef std::vector<const Parameter*> Parameters;
       const Channels& getChannels() const;
       const Channel* getChannel(unsigned int) const;
       const Parameters& getParameters() const;
       std::set<size_t> getParameterIndex(
               const PdServ::Parameter *p, size_t start, size_t nelem) const;
       const Parameters& getParameters(const PdServ::Parameter *p) const;
       const Parameter* getParameter(unsigned int) const;


    private:

        Channels channels;
        Parameters parameters;

        typedef std::vector<size_t> ParameterIndex;
        typedef std::map<size_t, ParameterIndex> ParameterStartIndex;
        typedef std::map<const PdServ::Parameter*, ParameterStartIndex>
            ParameterExtentMap;
        ParameterExtentMap extents;
};

}

#endif  // DIRECTORY_H
