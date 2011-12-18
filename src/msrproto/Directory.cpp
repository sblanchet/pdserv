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
#include "XmlElement.h"
#include "../Signal.h"
#include "../Parameter.h"
#include "../Debug.h"

#include <cstring>
#include <cerrno>
#include <locale>
#include <sstream>
#include <stack>
#include <assert.h>

#ifdef TRADITIONAL
    static const bool traditional = 1;
#else
    static const bool traditional = 0;
#endif

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
VariableDirectory::VariableDirectory()
{
    parent = this;
}

/////////////////////////////////////////////////////////////////////////////
VariableDirectory::~VariableDirectory()
{
}

/////////////////////////////////////////////////////////////////////////////
void VariableDirectory::list(
        PdServ::Session *session, XmlElement& parent, const char *path) const
{
    if (!path or *path != '/')
        return;

    DirectoryNode::list(session, parent, path+1);
}

/////////////////////////////////////////////////////////////////////////////
bool VariableDirectory::insert( const PdServ::Parameter *p)
{
    const char *path = p->path.c_str();

    if (*path++ != '/')
        return true;

    std::string name;
    char hide = 0;
    DirectoryNode *dir = mkdir(path, hide, name);
    if (!dir)
        return true;

    if (traditional and (hide == 1 or hide == 'p'))
        return false;

    Parameter *mainParam = new Parameter(parameters.size(), p);
    dir->insert(mainParam, name);

    parameterMap[p] = mainParam;
    parameters.push_back(mainParam);

    if (traditional and p->nelem > 1) {
        parameters.reserve(parameters.size() + p->nelem);

        for (size_t i = 0; i < p->nelem; ++i) {
            Parameter *parameter = new Parameter(parameters.size(), p, i);
            mainParam->insert(parameter, i, p->nelem, p->ndims, p->getDim());

            parameters.push_back(parameter);
            mainParam->addChild(parameter);
        }
    }

    return false;
}

/////////////////////////////////////////////////////////////////////////////
bool VariableDirectory::insert(const PdServ::Signal *s)
{
    const char *path = s->path.c_str();

    if (*path++ != '/')
        return true;

    std::string name;
    char hide = 0;
    DirectoryNode *dir = mkdir(path, hide, name);
    if (!dir)
        return true;

    if (traditional and (hide == 1 or hide == 's'))
        return false;

    if (traditional and s->nelem > 1) {
        channels.reserve(channels.size() + s->nelem);

        dir = dir->DirectoryNode::insert(new DirectoryNode, name);

        for (size_t i = 0; i < s->nelem; ++i) {
            Channel *channel = new Channel(s, channels.size(), i);
            dir->insert(channel, i, s->nelem, s->ndims, s->getDim());

            channels.push_back(channel);
        }
    }
    else {
        Channel *channel = new Channel(s, channels.size());
        dir->insert(channel, name);

        channels.push_back(channel);
    }

    return false;
}

/////////////////////////////////////////////////////////////////////////////
const Parameter *VariableDirectory::find( const PdServ::Parameter *p) const
{
    return parameterMap.find(p)->second;
}

/////////////////////////////////////////////////////////////////////////////
const VariableDirectory::Channels& VariableDirectory::getChannels() const
{
    return channels;
}

/////////////////////////////////////////////////////////////////////////////
const Channel* VariableDirectory::getChannel(size_t idx) const
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
DirectoryNode::DirectoryNode()
{
    hypernode = false;
}

/////////////////////////////////////////////////////////////////////////////
DirectoryNode::~DirectoryNode()
{
}

/////////////////////////////////////////////////////////////////////////////
void DirectoryNode::list(
        PdServ::Session *session, XmlElement& parent, const char *path) const
{
    char c;
    std::string name = split(path, c);

    Children::const_iterator it = children.find(name);
    if (!name.empty()) {
        if (it == children.end())
            return;
        else
            return it->second->list(session, parent, path);
    }

    for (it = children.begin(); it != children.end(); ++it) {

        if (it->second->hasChildren()) {
            XmlElement el("directory", parent);
            XmlElement::Attribute(el, "path")
                << (this->path() + '/' + it->first);
        }

        const Parameter *param = dynamic_cast<const Parameter *>(it->second);
        const Channel *channel = dynamic_cast<const Channel *>(it->second);
        if (param) {
            char buf[param->mainParam->memSize];
            struct timespec ts;

            param->mainParam->getValue(session, buf, &ts);

            XmlElement ("parameter", parent)
                .setAttributes(param, buf, ts, 0, 0, 16);
        }
        else if (channel) {
            XmlElement("channel", parent)
                .setAttributes(channel, 0, 0, 0);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
std::string DirectoryNode::path() const
{
    return parent == this ? std::string() : parent->path() + '/' + *name;
}

/////////////////////////////////////////////////////////////////////////////
bool DirectoryNode::hasChildren() const
{
    return !children.empty();
}

/////////////////////////////////////////////////////////////////////////////
DirectoryNode *DirectoryNode::find(const char *path) const
{
    char c;
    std::string name = split(path, c);

    Children::const_iterator it = children.find(name);
    if (name.empty() or it == children.end())
        return 0;

    return *path ? find(path) : it->second;
}

/////////////////////////////////////////////////////////////////////////////
DirectoryNode *DirectoryNode::push(std::string &name)
{
    DirectoryNode *dir = new DirectoryNode;

    if (hypernode) {
        std::ostringstream os;
        os << children.size();
        name = os.str();
    }
    else {
        this->hypernode = true;

        // Move all children into new directory
        for (Children::const_iterator it = children.begin();
                it != children.end(); ++it) {
            dir->insert(it->second, it->first);
        }

        children.clear();

        this->insert(dir, "0");

        // Create another directory
        dir = new DirectoryNode;
        name = "1";
    }

    this->insert(dir, name);

    return this;
}

/////////////////////////////////////////////////////////////////////////////
void DirectoryNode::dump() const
{
    debug() << path();
    for (Children::const_iterator it = children.begin();
            it != children.end(); ++it) {
        it->second->dump();
    }
}

/////////////////////////////////////////////////////////////////////////////
DirectoryNode *DirectoryNode::mkdir(
        const char *path, char &hide, std::string &name)
{
    name = split(path, hide);

    if (name.empty())
        return 0;

    DirectoryNode *node = children[name];

    if (*path) {
        // We're not at the end of the path yet
        if (!node)
            node = insert(new DirectoryNode, name);

        return node->mkdir(path, hide, name);
    }

    // Check whether the node exists
    if (node and traditional) {
        debug() << "double for" << this->path() << name;
        // Let the node choose a new name
        return node->push(name);
    }

    return this;
}

/////////////////////////////////////////////////////////////////////////////
DirectoryNode *DirectoryNode::insert(
        DirectoryNode *node, const std::string &name)
{
    children[name] = node;
    node->parent = this;
    node->name = &children.find(name)->first;

    return node;
}

/////////////////////////////////////////////////////////////////////////////
void DirectoryNode::insert(Variable *var, size_t index,
        size_t nelem, size_t ndims, const size_t *dim)
{
    nelem /= *dim++;
    size_t x = index / nelem;

    std::ostringstream os;
    os << x;
    std::string name(os.str());

    if (nelem == 1)
        insert(var, name);
    else {
        DirectoryNode *dir = children[name];
        if (!dir)
            dir = insert(new DirectoryNode, name);
        dir->insert( var, index - x*nelem , nelem, ndims - 1, dim);
    }
}

/////////////////////////////////////////////////////////////////////////////
std::string DirectoryNode::split(const char *&path, char &hide)
{
    // Skip whitespace at the beginning of the path
    while (std::isspace(*path, std::locale::classic()))
        if (!*++path)
            return std::string();

    // Find path separator
    const char *slash = path;
    while (*slash and *slash != '/')
        slash++;

    const char *nameEnd = slash;
    if (traditional) {
        nameEnd = ::strchr(path, '<');

        if (nameEnd == NULL)
            nameEnd = slash;
        else {
            XmlParser p(nameEnd, slash - nameEnd);

            const char *value;
            if (p.isTrue("hide"))
                hide = 1;
            else if (p.find("hide", value))
                hide = *value;

            if (p.isTrue("unhide"))
                hide = 0;
        }
    }

    while (nameEnd > path
            and std::isspace(nameEnd[-1], std::locale::classic()))
        --nameEnd;

    const char *begin = path;

    path = slash;
    while (*path and *++path == '/');

    return std::string(begin, nameEnd - begin);
}
