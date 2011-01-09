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
#include "../Signal.h"
#include "../Parameter.h"
#include "Channel.h"
#include "Parameter.h"
#include "XmlParser.h"
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
void makeExtension(std::string &path, size_t elementIdx, size_t nelem,
        const size_t *dim)
{
    std::ostringstream os;
    size_t x;

    while (nelem > 1) {
        nelem /= *dim++;
        x = elementIdx / nelem;
        elementIdx -= x*nelem;

        os << '/' << x;
    }

    path = os.str();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
DirectoryNode::DirectoryNode(): parent(0), channel(0), parameter(0)
{
}

/////////////////////////////////////////////////////////////////////////////
DirectoryNode::DirectoryNode(
        const DirectoryNode *parent, const std::string& _name):
    parent(parent), channel(0), parameter(0)
{
    size_t pos = _name.find('<');
    if (pos == std::string::npos)
        pos = _name.size();
    name.assign(_name, 0, pos);

    XmlParser n(_name);

    hide = (n.isTrue("hide") or parent->isHidden()) and !n.isTrue("unhide");
}

/////////////////////////////////////////////////////////////////////////////
DirectoryNode::~DirectoryNode()
{
    delete channel;
    delete parameter;
    for (Entry::const_iterator it = entry.begin();
            it != entry.end(); it++)
        delete it->second;
}

/////////////////////////////////////////////////////////////////////////////
std::string DirectoryNode::path() const
{
    return parent ? (parent->path() + '/' + name) : std::string();
}

/////////////////////////////////////////////////////////////////////////////
bool DirectoryNode::isHidden() const
{
    return hide;
}

/////////////////////////////////////////////////////////////////////////////
void DirectoryNode::insert( const HRTLab::Signal *s,
        std::list<const Channel *>& channelList, bool traditional,
        const char *path, size_t signalElement, size_t nelem)
{
    if (!parent) {
        path = s->path.c_str();
        signalElement = 0;
        nelem = s->nelem;
    }

    if (path) {
        std::string name = splitPath(path);
        if (!entry[name]) {
            entry[name] = new DirectoryNode(this, name);
        }
        return entry[name]->insert(
                s, channelList, traditional, path, signalElement, nelem);
    }

    if (hide)
        return;

    if (traditional and nelem > 1) {
        std::string extPath;
        for (size_t i = 0; i < nelem; i++) {
            makeExtension(extPath, i, nelem, s->getDim());
            insert(s, channelList, 0, extPath.c_str(), i, 1);
        }
        return;
    }

    if (channel or parameter)
        return;

    channel = new Channel( this, s, channelList.size(), signalElement, nelem);
    channelList.push_back(channel);
}

/////////////////////////////////////////////////////////////////////////////
void DirectoryNode::insert(const HRTLab::Parameter *mainParam,
        std::list<const Parameter *>& parameterList, bool traditional,
        const char *path, size_t parameterElement, size_t nelem)
{
    if (!parent) {
        path = mainParam->path.c_str();
        parameterElement = 0;
        nelem = mainParam->nelem;
    }

    if (path) {
        std::string name = splitPath(path);
        if (!entry[name]) {
            entry[name] = new DirectoryNode(this, name);
        }
        return entry[name]->insert( mainParam, parameterList, traditional,
                path, parameterElement, nelem);
    }

    if (hide)
        return;

    size_t vectorLen = mainParam->getDim()[mainParam->ndims - 1];
    if (traditional and nelem > vectorLen) {
        std::string extPath;
        size_t count = nelem / vectorLen;
        for (size_t i = 0; i < count; i++) {
            makeExtension(extPath, i, count, mainParam->getDim());
            insert(mainParam, parameterList, 0, extPath.c_str(),
                    i * vectorLen, vectorLen);
        }
        return;
    }

    if (channel or parameter)
        return;

    parameter = new Parameter(
            this, mainParam, parameterList.size(), nelem, parameterElement);
    parameterList.push_back(parameter);

    if (traditional or nelem == 1)
        return;

    std::string extPath;
    for (size_t i = 0; i < nelem; i++) {
        makeExtension(extPath, i, nelem, &nelem);
        insert(mainParam, parameterList, 0, extPath.c_str(),
                parameterElement + i, 1);
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
