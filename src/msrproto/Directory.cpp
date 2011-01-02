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

#include "Directory.h"
#include "../Variable.h"
#include "Channel.h"
#include "Parameter.h"
#include <cstring>
#include <cerrno>

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Node::Node(const Node *parent): parent(parent)

{
}

/////////////////////////////////////////////////////////////////////////////
std::string Node::path() const
{
    return parent ? (parent->path() + '/' + name) : std::string();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
DirectoryNode::DirectoryNode(
        const DirectoryNode *parent, const std::string& _name,
        Channel *c, Parameter *p, const char *variableName):
    Node(parent), channel(c), parameter(p)
{
    Node::name = _name;

    if (channel)
        c->setNode(this, variableName);
    else if (parameter)
        p->setNode(this, variableName);
}

/////////////////////////////////////////////////////////////////////////////
DirectoryNode::~DirectoryNode()
{
    for (Entry::const_iterator it = entry.begin();
            it != entry.end(); it++)
        delete it->second;
}

/////////////////////////////////////////////////////////////////////////////
bool DirectoryNode::insert(
        const HRTLab::Variable *variable, const char *extendedPath,
        Channel *channel, Parameter *parameter, const char *path,
        const char *variableName)
{
    std::string name;

    if (!parent)        // Root node
        path = variable->path.c_str();

//    cout << __func__
//        << " path=" << (path ? path : "nullPath")
//        << " extendedPath=" << (extendedPath ? extendedPath : "nullextendedPath")
//        << endl;

    if (path) {
        name = splitPath(path);
        variableName = name.c_str();
    }
    else if (extendedPath)
        name = splitPath(extendedPath);

//    cout << __func__
//        << " path=" << (path ? path : "nullPath")
//        << " extendedPath=" << (extendedPath ? extendedPath : "nullextendedPath")
//        << endl;


//    cout << __func__
//        << " name=" << name
//        << endl;
//
    if (path or (extendedPath and *extendedPath)) {
        DirectoryNode *dir = dynamic_cast<DirectoryNode*>(entry[name]);

        if (!entry[name]) {
            dir = new DirectoryNode(this, name);
            entry[name] = dir;
        }
        else if (!dir)
            return false;

        //cout << "Directory " << name << endl;
        return dir->insert( variable, extendedPath,
                channel, parameter, path, variableName);
    }
    else {
        if (entry[name])
            return false;

        entry[name] =
            new DirectoryNode(this, name, channel, parameter, variableName);

        return true;
    }
}

/////////////////////////////////////////////////////////////////////////////
const DirectoryNode *DirectoryNode::findVariable(const char *path) const
{
    std::string name = splitPath(path);
    Entry::const_iterator node = entry.find(name);

    if (node == entry.end())
        return 0;

    const DirectoryNode *dir =
        dynamic_cast<const DirectoryNode*>(node->second);

    return path ? dir->findVariable(path) : dir;
}


/////////////////////////////////////////////////////////////////////////////
const Channel *DirectoryNode::findChannel(const char *path) const
{
    const DirectoryNode *d = findVariable(path);
    return d ? d->channel : 0;
}

/////////////////////////////////////////////////////////////////////////////
const Parameter *DirectoryNode::findParameter(const char *path) const
{
    const DirectoryNode *d = findVariable(path);
    return d ? d->parameter : 0;
}

/////////////////////////////////////////////////////////////////////////////
std::string DirectoryNode::splitPath(const char *&path)
{
    const char *begin = *path == '/' ? path+1 : path;

    path = strchr(begin, '/');
    return path ? std::string(begin, path - begin) : std::string(begin);
}
