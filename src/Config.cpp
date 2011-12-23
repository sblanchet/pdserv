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

#include <cstring>
#include <cstdio>
#include <cerrno>

#include "Debug.h"
#include "Config.h"

#include <yaml.h>
using namespace PdServ;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static char error[100];

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Config::Config(): node(0)
{
}

/////////////////////////////////////////////////////////////////////////////
Config::Config(const Config &other):
    document(other.document), node(other.node)
{
}

/////////////////////////////////////////////////////////////////////////////
Config::Config(yaml_document_t *d, yaml_node_t *n): document(d), node(n)
{
}

/////////////////////////////////////////////////////////////////////////////
Config::~Config()
{
    if (!file.empty()) {
        yaml_document_delete(document);
        delete document;
    }
}

/////////////////////////////////////////////////////////////////////////////
const char * Config::load(const char *file)
{
    yaml_parser_t parser;
    FILE *fh;

    /* Initialize parser */
    if(!yaml_parser_initialize(&parser)) {
        ::snprintf(error, sizeof(error), "Could not initialize YAML parser");
        return error;
    }

    fh = ::fopen(file, "r");
    if (!fh) {
        ::snprintf(error, sizeof(error), "Could not open config file %s: %s",
                file, strerror(errno));
        return error;
    }

    /* Set input file */
    yaml_parser_set_input_file(&parser, fh);

    document = new yaml_document_t;

    /* START new code */
    if (!yaml_parser_load(&parser, document)) {
        snprintf(error, sizeof(error), "YAML parser failure at line %zu: %s",
            parser.problem_mark.line, parser.problem);
        return error;
    }

    node = yaml_document_get_root_node(document);

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
Config Config::operator[](const char *key) const
{
    return this->operator[](std::string(key));
}

/////////////////////////////////////////////////////////////////////////////
bool Config::operator!() const
{
    return node == 0;
}

/////////////////////////////////////////////////////////////////////////////
Config Config::operator[](const std::string& key) const
{
    if (!node or node->type != YAML_MAPPING_NODE)
        return Config();

    for (yaml_node_pair_t *pair = node->data.mapping.pairs.start;
            pair != node->data.mapping.pairs.top; ++pair) {
        yaml_node_t *n = yaml_document_get_node(document, pair->key);
        if (n->type == YAML_SCALAR_NODE
                and key.size() == n->data.scalar.length
                and !strncmp((char*)n->data.scalar.value,
                    key.c_str(), n->data.scalar.length)) {
            return Config(document,
                    yaml_document_get_node(document, pair->value));
        }
    }

    return Config();
}

/////////////////////////////////////////////////////////////////////////////
Config Config::operator[](size_t index) const
{
    if (!node or node->type != YAML_SEQUENCE_NODE)
        return Config();

    yaml_node_item_t *n = node->data.sequence.items.start;
    for (size_t i = 0; i < index; ++i) {
        if (n == node->data.sequence.items.top) {
            return Config();
        }
    }

    return Config(document, yaml_document_get_node(document, *n));
}

/////////////////////////////////////////////////////////////////////////////
Config::operator std::string() const
{
    if (!node or node->type != YAML_SCALAR_NODE)
        return std::string();

    return std::string((char*)node->data.scalar.value, node->data.scalar.length);
}

/////////////////////////////////////////////////////////////////////////////
Config::operator int() const
{
    std::string sval(*this);
    if (sval.empty())
        return 0;

    return strtol(sval.c_str(), 0, 0);
}

/////////////////////////////////////////////////////////////////////////////
Config::operator unsigned int() const
{
    std::string sval(*this);
    if (sval.empty())
        return 0;

    return strtoul(sval.c_str(), 0, 0);
}
