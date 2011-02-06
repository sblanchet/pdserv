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
#include <list>

namespace HRTLab {
    class Variable;
}

namespace MsrProto {

class Session;
class Channel;
class Parameter;

/////////////////////////////////////////////////////////////////////////////
class DirectoryNode {
    public:
        DirectoryNode();
        DirectoryNode( const DirectoryNode *parent,
                const std::string &name, bool hide);
        ~DirectoryNode();

        std::string path() const;

        std::string name;
        const bool hide;

        const Channel *findChannel(const char *path) const;
        const Parameter *findParameter(const char *path) const;

        DirectoryNode *mkdir(const HRTLab::Variable *v);
        DirectoryNode *mkdir(const HRTLab::Variable *v,
                unsigned int idx, bool vector);
        void insert(const Parameter *p);
        void insert(const Channel *p);

    private:
        const DirectoryNode *parent;

        bool empty() const;
        DirectoryNode *mkdir(const char *path);
        DirectoryNode *mkdir(size_t idx,
                size_t nelem, const size_t *dim, size_t ndim);

        static std::string splitPath(const char *&path);

        typedef std::map<const std::string, DirectoryNode*> Entry;
        Entry entry;

        const Channel *channel;
        const Parameter *parameter;

        const DirectoryNode *findVariable(const char *path) const;
};

}

#endif  // DIRECTORY_H
