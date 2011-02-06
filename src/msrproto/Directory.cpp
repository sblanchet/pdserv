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
DirectoryNode::DirectoryNode(): name(std::string()), hide(false),
    parent(0), channel(0), parameter(0)
{
}

/////////////////////////////////////////////////////////////////////////////
DirectoryNode::DirectoryNode( const DirectoryNode *parent,
        const std::string& _name, bool hide):
    name(_name), hide(hide),
    parent(parent), channel(0), parameter(0)
{
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
void DirectoryNode::insert(const Channel *c)
{
    channel = c;
}

/////////////////////////////////////////////////////////////////////////////
void DirectoryNode::insert(const Parameter *p)
{
    parameter = p;
}

/////////////////////////////////////////////////////////////////////////////
bool DirectoryNode::empty() const
{
    return !channel and !parameter;
}

/////////////////////////////////////////////////////////////////////////////
DirectoryNode *DirectoryNode::mkdir(size_t idx,
        size_t nelem, const size_t *dim, size_t ndim)
{
    if (!ndim)
        return this;

    nelem /= *dim++;
    size_t x = idx / nelem;

    std::ostringstream os;
    os << x;
    std::string name(os.str());

    if (!entry[name])
        entry[name] = new DirectoryNode(this, name, hide);

    return entry[name]->mkdir(idx - x*nelem, nelem, dim, ndim-1);
}

/////////////////////////////////////////////////////////////////////////////
DirectoryNode *DirectoryNode::mkdir(const char *path)
{
    if (!path)
        return this;

    std::string name = splitPath(path);

    bool _hide = this->hide;
    size_t xmlpos = name.find('<');
    if (xmlpos != std::string::npos) {
        XmlParser p(name, xmlpos);
        _hide = p.isTrue("hide") or (this->hide and !p.isTrue("unhide"));

        name.erase(xmlpos);
    }

    if (!entry[name]) {
        entry[name] = new DirectoryNode(this, name, _hide);
    }

    return entry[name]->mkdir(path);
}

/////////////////////////////////////////////////////////////////////////////
DirectoryNode *DirectoryNode::mkdir(const HRTLab::Variable *v,
        unsigned int idx, bool vector)
{
    DirectoryNode *d = mkdir(v->path.c_str());
    const size_t *dim = v->getDim();

    if (v->nelem > 1)
        d = d->mkdir(idx, v->nelem, dim, v->ndims - (vector ? 1 : 0));

    return d->empty() and !d->hide ? d : 0;
}

/////////////////////////////////////////////////////////////////////////////
DirectoryNode *DirectoryNode::mkdir(const HRTLab::Variable *v)
{
    DirectoryNode *d = mkdir(v->path.c_str());
    return d->empty() and !d->hide ? d : 0;
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
