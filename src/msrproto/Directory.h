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

#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <string>
#include <map>

namespace HRTLab {
    class Variable;
}

namespace MsrProto {

class Session;
class Channel;
class Parameter;

/////////////////////////////////////////////////////////////////////////////
class Node {
    public:
        Node(const Node *parent = 0);

        virtual std::string path() const;

    protected:
        const Node *parent;

        std::string name;
};

/////////////////////////////////////////////////////////////////////////////
class DirectoryNode: public Node {
    public:
        DirectoryNode( const DirectoryNode *parent, const std::string &name,
                Channel *channel = 0, Parameter *parameter = 0,
                const char *variableName = 0);
        ~DirectoryNode();

        bool insert( const HRTLab::Variable *, const char *extendedPath,
                Channel *c, Parameter *p,
                const char *path = 0, const char *variableName = 0);

        const Channel *findChannel(const char *path) const;
        const Parameter *findParameter(const char *path) const;

    private:
        static std::string splitPath(const char *&path);

        typedef std::map<const std::string, Node*> Entry;
        Entry entry;

        const Channel * const channel;
        const Parameter * const parameter;

        const DirectoryNode *findVariable(const char *path) const;
};

}

#endif  // DIRECTORY_H
