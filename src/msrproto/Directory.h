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
    class Signal;
    class Parameter;
}

namespace MsrProto {

class Session;
class Channel;
class Parameter;

/////////////////////////////////////////////////////////////////////////////
class DirectoryNode {
    public:
        DirectoryNode();
        DirectoryNode( const DirectoryNode *parent, const std::string &name);
        ~DirectoryNode();

        std::string path() const;
        bool isHidden() const;

        const Channel *findChannel(const char *path) const;
        const Parameter *findParameter(const char *path) const;

        void insert(const HRTLab::Signal *s,
                std::list<const Channel *>& channelList,
                bool traditional, const char *path = 0,
                size_t sigElem = 0, size_t nelem = 0);
        void insert(const HRTLab::Parameter *p,
                std::list<const Parameter *>& parameterList,
                bool traditional, const char *path = 0,
                size_t paramElem = 0, size_t nelem = 0);

    private:
        const DirectoryNode *parent;
        std::string name;
        bool hide;

        static std::string splitPath(const char *&path);

        typedef std::map<const std::string, DirectoryNode*> Entry;
        Entry entry;

        const Channel *channel;
        const Parameter *parameter;

        const DirectoryNode *findVariable(const char *path) const;
};

}

#endif  // DIRECTORY_H
