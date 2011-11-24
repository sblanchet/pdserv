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

#include "Directory.h"
#include "Channel.h"
#include "Parameter.h"
#include "XmlParser.h"
#include "../Signal.h"
#include "../Parameter.h"
#include "../Debug.h"

#include <cstring>
#include <cerrno>
#include <locale>
#include <sstream>

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
VariableDirectory::VariableDirectory()
{
}

/////////////////////////////////////////////////////////////////////////////
bool VariableDirectory::insert( const PdServ::Parameter *p, bool traditional)
{
    char hide = 0;
    DirectoryNode *dir = mkdir(p, hide);
    if (!dir)
        return true;

    if (hide == 1 or hide == 'p') {
//        debug() << "hide=" << hide << p->path;
        return false;
    }

    Parameter *mainParam = new Parameter(dir, parameters.size(), p);
    dir->insert(mainParam);
    parameterMap[p] = mainParam;
    parameters.push_back(mainParam);

    if (traditional and p->nelem > 1) {
        parameters.reserve(parameters.size() + p->nelem);

        for (size_t i = 0; i < p->nelem; ++i) {
            DirectoryNode *subdir =
                dir->mkdir(i, p->nelem, p->ndims, p->getDim());
            if (!subdir)
                continue;

            Parameter *parameter =
                new Parameter(subdir, parameters.size(), p, i);
            subdir->insert(parameter);
            parameters.push_back(parameter);
            mainParam->addChild(parameter);
        }
    }

    return false;
}

/////////////////////////////////////////////////////////////////////////////
bool VariableDirectory::insert(const PdServ::Signal *s, bool traditional)
{
    char hide = 0;
    DirectoryNode *dir = mkdir(s, hide);
    if (!dir)
        return true;

    if (hide == 1 or hide == 'k') {
//        debug() << "hide=" << hide << s->path;
        return false;
    }

    if (traditional) {
        channels.reserve(channels.size() + s->nelem);
        for (size_t i = 0; i < s->nelem; ++i) {
            DirectoryNode *subdir =
                dir->mkdir(i, s->nelem, s->ndims, s->getDim());
            if (!subdir)
                continue;
            Channel *c = new Channel(subdir, s, channels.size(), i);
            subdir->insert(c);
            channels.push_back(c);
        }
    }
    else {
        Channel *c = new Channel(dir, s, channels.size());
        dir->insert(c);
        channels.push_back(c);
    }

    return false;
}

/////////////////////////////////////////////////////////////////////////////
const Parameter *VariableDirectory::getParameter(
        const PdServ::Parameter *p) const
{
    return parameterMap.find(p)->second;
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
    parent(0), name(std::string())
{
}

/////////////////////////////////////////////////////////////////////////////
DirectoryNode::DirectoryNode(
        DirectoryNode *parent, const std::string& name):
    parent(parent), name(name)
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
        dir = new DirectoryNode(this, name);
        entry[name] = dir;
    }

    return dir->mkdir(index - x*nelem , nelem, ndims - 1, dim);
}

/////////////////////////////////////////////////////////////////////////////
DirectoryNode* DirectoryNode::mkdir(
        const PdServ::Variable *v, char& hide, size_t pathNameOffset)
{
    if (v->path[pathNameOffset++] != '/')
        return 0;
    
    size_t nameStart = pathNameOffset;

    pathNameOffset = v->path.find('/', pathNameOffset);
    if (pathNameOffset == std::string::npos)
        pathNameOffset = v->path.size();

    std::string name(v->path.substr(nameStart, pathNameOffset - nameStart));

    size_t xmlpos = name.find('<');
    if (xmlpos != std::string::npos) {
        XmlParser p(name, xmlpos);
        const char *value;

        if (p.isTrue("hide"))
            hide = 1;
        else if (p.find("hide", value))
            hide = *value;

        if (p.isTrue("unhide"))
            hide = 0;

        name.erase(xmlpos);
    }

    size_t end = name.size() - 1;
    while (end and std::isspace(name[end], std::locale::classic()))
        --end;
    name.erase(end + 1);

    DirectoryNode *dir = entry[name];
    if (!dir) {
        dir = new DirectoryNode(this, name);
        entry[name] = dir;
    }

    return pathNameOffset == v->path.size()
        ? dir : dir->mkdir(v, hide, pathNameOffset);
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
