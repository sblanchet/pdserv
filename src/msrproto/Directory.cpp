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

#include "config.h"

#include "Directory.h"
#include "Channel.h"
#include "Parameter.h"
#include "../Signal.h"
#include "../Parameter.h"
#include "XmlParser.h"
#include <cstring>
#include <cerrno>
#include <locale>

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
VariableDirectory::VariableDirectory()
{
}

/////////////////////////////////////////////////////////////////////////////
bool VariableDirectory::insert(
        const PdServ::Parameter *p, const std::string &path,
        size_t index, size_t elementCount, bool dependent)
{
    DirectoryNode *dir = mkdir(p, path, index, elementCount);
    if (!dir)
        return true;

    if (dir->hidden)
        return false;

    size_t n = parameters.size();
    Parameter *parameter =
        new Parameter(dir, dependent, p, n, elementCount, index);
    dir->insert(parameter);

    ParameterStartIndex& pi = extents[p];
    if (!index) {
        size_t n = p->nelem / elementCount;
        parameters.reserve(parameters.size() + n);

        pi.resize(pi.size() + 1);
        pi[pi.size() - 1].reserve(p->nelem / elementCount);
    }

    parameters.push_back(parameter);

    pi[elementCount == 1 and pi.size() == 2].push_back(n);

    return false;
}

/////////////////////////////////////////////////////////////////////////////
bool VariableDirectory::insert(const PdServ::Signal *s,
        const std::string &path, size_t index, size_t elementCount)
{
    if (!elementCount)
        elementCount = s->nelem;

    DirectoryNode *dir = mkdir(s, path, index, elementCount);
    if (!dir)
        return true;

    if (dir->hidden)
        return false;

    Channel *c = new Channel(dir, s, channels.size(), index, elementCount);
    dir->insert(c);

    if (!index)
        channels.reserve(channels.size() + s->nelem / elementCount);

    channels.push_back(c);

    return false;
}

/////////////////////////////////////////////////////////////////////////////
std::set<size_t> VariableDirectory::getParameterIndex(
        const PdServ::Parameter *p, size_t start, size_t nelem) const
{
    std::set<size_t> indices;
    const ParameterStartIndex& pi = extents.find(p)->second;

    if (pi.size() == 2) {
        size_t vectorLen = p->getDim()[p->ndims - 1];

        for (size_t i = start / vectorLen;
                i < (start + nelem) / vectorLen; ++i)
            indices.insert(pi[0][i]);

        for (size_t i = start; i < start + nelem; ++i)
            indices.insert(pi[1][i]);
    }
    else
        indices.insert(pi[0][0]);

    return indices;
}

/////////////////////////////////////////////////////////////////////////////
const VariableDirectory::Channels& VariableDirectory::getChannels() const
{
    return channels;
}

/////////////////////////////////////////////////////////////////////////////
const Channel* VariableDirectory::getChannel(unsigned int idx) const
{
    return idx < channels.size() ? channels[idx] : 0;
}

/////////////////////////////////////////////////////////////////////////////
const VariableDirectory::Parameters& VariableDirectory::getParameters() const
{
    return parameters;
}

/////////////////////////////////////////////////////////////////////////////
const Parameter* VariableDirectory::getParameter(unsigned int idx) const
{
    return idx < parameters.size() ? parameters[idx] : 0;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
DirectoryNode::DirectoryNode():
    parent(0), name(std::string()), hidden(false)
{
}

/////////////////////////////////////////////////////////////////////////////
DirectoryNode::DirectoryNode(
        DirectoryNode *parent, const std::string& name, bool hide):
    parent(parent), name(name), hidden(hide)
{
}

/////////////////////////////////////////////////////////////////////////////
void DirectoryNode::insert(const Variable *v)
{
    variable = v;
}

/////////////////////////////////////////////////////////////////////////////
DirectoryNode* DirectoryNode::mkdir(size_t index,
                size_t nelem, size_t ndims, const size_t *dim)
{
    if (nelem == 1)
        return this;

    nelem /= *dim++;
    size_t x = index / nelem;

    std::ostringstream os;
    os << x;
    std::string name(os.str());

    DirectoryNode *dir = entry[name];
    if (!dir) {
        dir = new DirectoryNode(this, name, hidden);
        entry[name] = dir;
    }

    return dir->mkdir(index - x*nelem , nelem, ndims - 1, dim);
}

/////////////////////////////////////////////////////////////////////////////
DirectoryNode* DirectoryNode::mkdir(
        const PdServ::Variable *v, const std::string &path,
        size_t index, size_t elementCount, size_t pathNameOffset)
{
    if (path.size() == pathNameOffset)
        return mkdir(index / elementCount, v->nelem / elementCount,
                v->ndims, v->getDim());

    if (path[pathNameOffset++] != '/')
        return 0;
    
    size_t nameStart = pathNameOffset;

    pathNameOffset = path.find('/', pathNameOffset);
    if (pathNameOffset == std::string::npos)
        pathNameOffset = path.size();

    std::string name(path.substr(nameStart, pathNameOffset - nameStart));

    bool hide = hidden;

    size_t xmlpos = name.find('<');
    if (xmlpos != std::string::npos) {
        XmlParser p(name, xmlpos);
        hide = p.isTrue("hide") or (hide and !p.isTrue("unhide"));

        name.erase(xmlpos);
    }

    DirectoryNode *dir = entry[name];
    if (!dir) {
        dir = new DirectoryNode(this, name, hide);
        entry[name] = dir;
    }

    return dir->mkdir(v, path, index, elementCount, pathNameOffset);
}

/////////////////////////////////////////////////////////////////////////////
std::string DirectoryNode::path() const
{
    return parent ? (parent->path() + '/' + name) : std::string();
}

/////////////////////////////////////////////////////////////////////////////
const Variable *DirectoryNode::findVariable(
        const std::string &path, size_t pathOffset) const
{
    if (pathOffset >= path.size()
            or (!pathOffset and path[pathOffset++] != '/'))
        return 0;

    size_t pos = path.find('/', pathOffset);
    if (pos == std::string::npos)
        pos = path.size();

    Entry::const_iterator node =
        entry.find(path.substr(pathOffset, pos - pathOffset));

    if (node == entry.end())
        return 0;

    const DirectoryNode *dir = node->second;

    return path.size() == pos
        ? dir->variable : dir->findVariable(path, pos + 1);
}


/////////////////////////////////////////////////////////////////////////////
const Channel *DirectoryNode::findChannel(const std::string &path) const
{
    const Variable *v = findVariable(path, 0);
    return v and v->signal() ? static_cast<const Channel*>(v) : 0;
}

/////////////////////////////////////////////////////////////////////////////
const Parameter *DirectoryNode::findParameter(const std::string &path) const
{
    const Variable *v = findVariable(path, 0);
    return v and v->parameter() ? static_cast<const Parameter*>(v) : 0;
}
