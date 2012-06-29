/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdserv library.
 *
 *  The pdserv library is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or (at
 *  your option) any later version.
 *
 *  The pdserv library is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with the pdserv library. If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include "config.h"

#include "Server.h"
#include "Directory.h"
#include "Channel.h"
#include "Parameter.h"
#include "XmlParser.h"
#include "XmlElement.h"
#include "../Signal.h"
#include "../Parameter.h"
#include "../Debug.h"
#include "../DataType.h"

#include <cstring>
#include <cerrno>
#include <locale>
#include <sstream>
#include <iostream>

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
VariableDirectory::VariableDirectory()
{
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
DirectoryNode *VariableDirectory::mkdir(std::string& path,
        char &hide, bool traditional)
{
    const char *p = path.c_str();

    if (*p++ != '/')
        return 0;

    std::string name;
    hide = 0;

    DirectoryNode *dir = DirectoryNode::mkdir(p, hide, name, traditional);
    path = name;

    return dir;
}

/////////////////////////////////////////////////////////////////////////////
void VariableDirectory::insertChildren(const Channel *c)
{
    const Variable::List* children = c->getChildren();
    if (!children)
        return;

    channels.reserve(channels.size() + children->size());
    for (Variable::List::const_iterator it = children->begin();
            it != children->end();  ++it) {
        c = static_cast<const Channel*>(*it);
        channels.push_back(c);
        insertChildren(c);
    }
}

/////////////////////////////////////////////////////////////////////////////
void VariableDirectory::insert(const PdServ::Signal *signal,
        std::string path, bool traditional)
{
    char hide;

    path.append(signal->path);

    DirectoryNode *dir = mkdir(path, hide, traditional);
    if (dir and !(traditional and (hide == 1 or hide == 'k'))) {
        Channel *c =
            new Channel(signal, path, dir, channels.size(), traditional);

        channels.push_back(c);
        insertChildren(c);
    }
}

/////////////////////////////////////////////////////////////////////////////
void VariableDirectory::insertChildren(const Parameter *p)
{
    const Variable::List* children = p->getChildren();
    if (!children)
        return;

    parameters.reserve(parameters.size() + children->size());
    for (Variable::List::const_iterator it = children->begin();
            it != children->end();  ++it) {
        p = static_cast<const Parameter*>(*it);
        parameters.push_back(p);
        insertChildren(p);
    }
}

/////////////////////////////////////////////////////////////////////////////
void VariableDirectory::insert(const PdServ::Parameter *param,
        std::string path, bool traditional)
{
    char hide;

    path.append(param->path);

    DirectoryNode *dir = mkdir(path, hide, traditional);
    if (dir and !(traditional and (hide == 1 or hide == 'p'))) {
        Parameter *p =
            new Parameter(param, path, dir, parameters.size(), traditional);
        parameters.push_back(p);
        parameterMap[param] = p;
        insertChildren(p);
    }
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
const Parameter* VariableDirectory::getParameter(size_t idx) const
{
    return idx < parameters.size() ? parameters[idx] : 0;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
DirectoryNode::DirectoryNode(const std::string& name, DirectoryNode* parent):
    parent(parent)
{
    rename(parent, name);
}

/////////////////////////////////////////////////////////////////////////////
DirectoryNode::DirectoryNode(): parent(this), name(0)
{
}

/////////////////////////////////////////////////////////////////////////////
DirectoryNode::~DirectoryNode()
{
}

/////////////////////////////////////////////////////////////////////////////
void DirectoryNode::rename(DirectoryNode* parent, const std::string& name)
{
    this->parent = parent;
    this->name = parent->insert(this, name);
}

/////////////////////////////////////////////////////////////////////////////
void DirectoryNode::list(
        PdServ::Session *session, XmlElement& parent, const char *path) const
{
    char c;
    std::string name = split(path, c, false);

    Children::const_iterator it = children.find(name);
    if (!name.empty()) {
        if (it == children.end())
            return;
        else
            return it->second->list(session, parent, path);
    }

    for (it = children.begin(); it != children.end(); ++it) {

        // Must check whether second exists, in case hidden variables were not
        // written to this node
        if (it->second and it->second->hasChildren()) {
            XmlElement el("dir", parent);
            XmlElement::Attribute(el, "path")
                << (this->path() + '/' + it->first);
        }

        const Parameter *param = dynamic_cast<const Parameter *>(it->second);
        const Channel *channel = dynamic_cast<const Channel *>(it->second);
        if (param) {
            char buf[param->mainParam->memSize];
            struct timespec ts;

            param->mainParam->getValue(session, buf, &ts);

            XmlElement xml("parameter", parent);
            param->setXmlAttributes(xml, buf, ts, 0, 0, 16);
        }
        else if (channel) {
            XmlElement xml("channel", parent);
            channel->setXmlAttributes(xml, 0, 0, 0);
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
    std::string name = split(path, c, false);

    Children::const_iterator it = children.find(name);
    if (name.empty() or it == children.end())
        return 0;

    return *path ? it->second->find(path) : it->second;
}

/////////////////////////////////////////////////////////////////////////////
void DirectoryNode::dump() const
{
    //cerr_debug() << path();
    for (Children::const_iterator it = children.begin();
            it != children.end(); ++it) {
        it->second->dump();
    }
}

/////////////////////////////////////////////////////////////////////////////
DirectoryNode *DirectoryNode::mkdir(
        const std::string& path, std::string& name)
{
    const char *p = path.c_str();

    if (*p++ != '/')
        return 0;

    char hide;

    return DirectoryNode::mkdir(p, hide, name, false);
}

/////////////////////////////////////////////////////////////////////////////
DirectoryNode *DirectoryNode::mkdir(
        const char *path, char &hide, std::string &name, bool traditional)
{
    name = split(path, hide, traditional);

    if (name.empty())
        return 0;

    DirectoryNode *node = children[name];

    if (*path) {
        // We're not at the end of the path yet
        if (!node)
            node = new DirectoryNode(name, this);

        return node->mkdir(path, hide, name, traditional);
    }

    // Check whether the node exists
    if (node) {
        log_debug("node %s exists", name.c_str());
        if (!traditional)
            return 0;

        if (reinterpret_cast<const Channel*>(node)
                or reinterpret_cast<const Parameter*>(node)) {

            DirectoryNode* dir = new DirectoryNode(name, this);
            node->rename(dir, "0");
            name = "1";
            return dir;
        }
        std::ostringstream os;
        os << children.size();
        name = os.str();

        return this;
    }

    return this;
}

/////////////////////////////////////////////////////////////////////////////
const std::string* DirectoryNode::insert (DirectoryNode *child,
        const std::string& name)
{
    children[name] = child;
    return &children.find(name)->first;
}

/////////////////////////////////////////////////////////////////////////////
std::string DirectoryNode::split(const char *&path,
        char &hide, bool traditional) const
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
        nameEnd = path;
        while (nameEnd < slash and *nameEnd != '<')
            nameEnd++;

        if (nameEnd != slash) {
            XmlParser p(nameEnd, slash);

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
    }

    while (nameEnd > path
            and std::isspace(nameEnd[-1], std::locale::classic()))
        --nameEnd;

    const char *begin = path;

    path = slash;
    while (*path and *++path == '/');

    return std::string(begin, nameEnd - begin);
}
