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
#include "XmlParser.h"
#include "../Signal.h"
#include "../Parameter.h"
#include "../Debug.h"

#include <cstring>
#include <cerrno>
#include <locale>
#include <sstream>
#include <stack>

#ifdef TRADITIONAL
    static const bool traditional = 1;
#else
    static const bool traditional = 0;
#endif

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
VariableDirectory::VariableDirectory()
{
}

/////////////////////////////////////////////////////////////////////////////
bool VariableDirectory::insert( const PdServ::Parameter *p)
{
    char hide = 0;
    DirectoryNode *dir = Path(p->path).create(this, hide);
    if (!dir)
        return true;

    if (traditional and (hide == 1 or hide == 'p')) {
        //debug() << "hide=" << hide << p->path << dir->path();
        return false;
    }

    Parameter *mainParam = new Parameter(parameters.size(), p);
    dir->insert(mainParam);
    parameterMap[p] = mainParam;
    parameters.push_back(mainParam);

    if (traditional and p->nelem > 1) {
        parameters.reserve(parameters.size() + p->nelem);

        for (size_t i = 0; i < p->nelem; ++i) {
            DirectoryNode *subdir =
                dir->create(i, p->nelem, p->ndims, p->getDim());
            if (!subdir)
                continue;

            Parameter *parameter = new Parameter(parameters.size(), p, i);
            subdir->insert(parameter);
            parameters.push_back(parameter);
            mainParam->addChild(parameter);
        }
    }

    return false;
}

/////////////////////////////////////////////////////////////////////////////
bool VariableDirectory::insert(const PdServ::Signal *s)
{
    char hide = 0;
    DirectoryNode *dir = Path(s->path).create(this, hide);
    if (!dir) {
        return true;
    }

    if (traditional and (hide == 1 or hide == 'k')) {
        //debug() << "hide=" << hide << s->path << dir->path();
        return false;
    }

    if (traditional) {
        channels.reserve(channels.size() + s->nelem);
        for (size_t i = 0; i < s->nelem; ++i) {
            DirectoryNode *subdir =
                dir->create(i, s->nelem, s->ndims, s->getDim());
            if (!subdir)
                continue;
            Channel *c = new Channel(s, channels.size(), i);
            subdir->insert(c);
            channels.push_back(c);
        }
    }
    else {
        Channel *c = new Channel(s, channels.size());
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
    parent(this), name(std::string()), variable(0), count(~0U)
{
}

/////////////////////////////////////////////////////////////////////////////
DirectoryNode::DirectoryNode(
        DirectoryNode *parent, const std::string& name):
    parent(parent), name(name), variable(0), count(~0U)
{
}

/////////////////////////////////////////////////////////////////////////////
void DirectoryNode::insert(const Variable *v)
{
    if (traditional) {
        if (count != ~0U) {
            // Indexed variable insertion
            std::ostringstream os;
            os << count++;
            DirectoryNode *dir = new DirectoryNode(this, os.str());
            dir->insert(v);
            v->directory = dir;
            //debug() << "leel " << count << "for" << dir->path();
            return;
        }
        else if (variable) {
            // Variable exists, so now insert them with indices
            count = 0;

            this->insert(variable);
            variable = 0;

            this->insert(v);
            return;
        }
        else
            variable = v;
    }
    else
        variable = v;

    v->directory = this;
}

/////////////////////////////////////////////////////////////////////////////
DirectoryNode* DirectoryNode::create(size_t index,
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

    return dir->create(index - x*nelem , nelem, ndims - 1, dim);
}

/////////////////////////////////////////////////////////////////////////////
DirectoryNode* DirectoryNode::getRoot()
{
    DirectoryNode* node = this;
    while (node != node->parent)
        node = node->parent;

    return node;
}

/////////////////////////////////////////////////////////////////////////////
std::string DirectoryNode::path() const
{
    return this == parent
        ? std::string()
        : parent->path().append(1,'/').append(name);
}

/////////////////////////////////////////////////////////////////////////////
const Channel *DirectoryNode::findChannel(const std::string &p) const
{
    return Path(p).findChannel(this);
}

/////////////////////////////////////////////////////////////////////////////
const Parameter *DirectoryNode::findParameter(const std::string &p) const
{
    return Path(p).findParameter(this);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
DirectoryNode::Path::Path(const std::string &s):
    path(s), offset(0)
{
}

/////////////////////////////////////////////////////////////////////////////
const Variable *DirectoryNode::Path::findVariable(const DirectoryNode* dir)
{
    std::string dirName;
    char hide;

    while (getDir(dirName, hide)) {
        if (!dirName.compare(0, 2, ".."))
            dir = dir->parent;
        else if (dirName.size() and dirName.compare(0, 1, ".")) {
            Entry::const_iterator it = dir->entry.find(dirName);
            if (it == dir->entry.end())
                return 0;
            dir = it->second;
        }
    }

    return dir->variable;
}

/////////////////////////////////////////////////////////////////////////////
const Parameter *DirectoryNode::Path::findParameter(const DirectoryNode* dir)
{
    const Variable *v = findVariable(dir);
    return v and v->parameter() ? static_cast<const Parameter*>(v) : 0;
}

/////////////////////////////////////////////////////////////////////////////
const Channel *DirectoryNode::Path::findChannel(const DirectoryNode* dir)
{
    const Variable *v = findVariable(dir);
    return v and v->signal() ? static_cast<const Channel*>(v) : 0;
}

/////////////////////////////////////////////////////////////////////////////
bool DirectoryNode::Path::getDir(std::string& name, char &hide)
{
    // Skip whitespace at the beginning of the path
    while (offset < path.size()
            and std::isspace(path[offset], std::locale::classic()))
        offset++;

    // Finished?
    if (offset == path.size())
        return false;

    size_t start = offset;
    
    offset = path.find('/', offset);
    if (offset == path.npos) {
        name.assign(path, start, path.size() - start);
        offset = path.size();
    }
    else
        name.assign(path, start, offset++ - start);

    size_t n;
    if (traditional) {
        n = name.find('<');
        if (n == name.npos)
            n = name.size();

        XmlParser p(name, n);

        if (p.next()) {
            const char *value;
            if (p.isTrue("hide"))
                hide = 1;
            else if (p.find("hide", value))
                hide = *value;

            if (p.isTrue("unhide"))
                hide = 0;
        }
    }
    else
        n = name.size();

    while (n and std::isspace(name[n-1], std::locale::classic()))
        n--;

    name.erase(n);

    return true;
}

/////////////////////////////////////////////////////////////////////////////
DirectoryNode *DirectoryNode::Path::create(DirectoryNode* dir, char &hide)
{
    std::string dirName;

    while (getDir(dirName, hide)) {
        if (!dirName.compare(0, 2, ".."))
            dir = dir->parent;
        else if (dirName.size() and dirName.compare(0, 1, ".")) {
            DirectoryNode *d = dir;
            dir = dir->entry[dirName];
            if (!dir) {
                dir = new DirectoryNode(d, dirName);

                d->entry[dirName] = dir;
            }
        }
    }

    return dir;
}
