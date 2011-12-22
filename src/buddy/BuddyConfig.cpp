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

#include "../Debug.h"

#include <cerrno>
#include <cstring>

#include "BuddyConfig.h"
#include <yaml.h>

/////////////////////////////////////////////////////////////////////////////
BuddyConfigRef::BuddyConfigRef(yaml_document_t *doc, yaml_node_t *node):
    node(node), document(doc)
{
}

/////////////////////////////////////////////////////////////////////////////
bool BuddyConfigRef::get(const std::string *name, std::string &value)
{
    return false;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
BuddyConfig::BuddyConfig()
{
    document = 0;
    root = 0;
}

/////////////////////////////////////////////////////////////////////////////
BuddyConfig::~BuddyConfig()
{
    if (document) {
        yaml_document_delete(document);
        delete document;
    }
}

/////////////////////////////////////////////////////////////////////////////
int BuddyConfig::load(const std::string &config_file)
{
    yaml_parser_t parser;
    FILE *fh;

    /* Initialize parser */
    if(!yaml_parser_initialize(&parser)) {
        std::cerr << "Could not initialize YAML parser" << std::endl;
        return 1;
    }

    fh = fopen(config_file.c_str(), "r");
    if (!fh) {
        std::cerr << "Could not open config file "
            << config_file << ": " << strerror(errno) << std::endl;
        return 1;
    }

    /* Set input file */
    yaml_parser_set_input_file(&parser, fh);

    document = new yaml_document_t;

    /* START new code */
    if (!yaml_parser_load(&parser, document)) {
        std::cerr << "YAML parser failure at line "
            << parser.problem_mark.line
            << ": " << parser.problem << std::endl;
        return 1;
    }

    root = yaml_document_get_root_node(document);

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
BuddyConfigRef BuddyConfig::select(const char *module)
{
    if (!root or root->type != YAML_MAPPING_NODE)
        return BuddyConfigRef();

    yaml_node_t *node;
    for (yaml_node_pair_t *pair = root->data.mapping.pairs.start;
            pair != root->data.mapping.pairs.top; ++pair) {
        node = yaml_document_get_node(document, pair->key);
        if (node->type == YAML_SCALAR_NODE
                and !strncmp((char*)node->data.scalar.value,
                    module, node->data.scalar.length)) {
            return BuddyConfigRef(document, node);
        }
    }

    return BuddyConfigRef();
}

// /////////////////////////////////////////////////////////////////////////////
// int BuddyConfig::load_seq(const std::string &config_file)
// {
//     std::string key;
//     yaml_parser_t parser;
//     yaml_event_t event;
//     FILE *fh;
//     int err = 0;
//     bool done;
//     unsigned int mappingLevel = 0, sequenceLevel = 0;
//     enum {
//         SECTION_START, CHECK_SECTION_NAME, READ_MAPPING, READ_KEY, READ_VALUE
//     } state = SECTION_START;
// 
//     this->config_file = config_file;
// 
//     /* Initialize parser */
//     if(!yaml_parser_initialize(&parser)) {
//         std::cerr << "Could not initialize YAML parser" << std::endl;
//         return 1;
//     }
// 
//     fh = fopen(config_file.c_str(), "r");
//     if (!fh) {
//         std::cerr << "Could not open config file "
//             << config_file << ": " << strerror(errno) << std::endl;
//         return 1;
//     }
// 
//     /* Set input file */
//     yaml_parser_set_input_file(&parser, fh);
// 
//     /* START new code */
//     do {
//         if (!yaml_parser_parse(&parser, &event)) {
//             std::cerr << "YAML parser failure at line "
//                 << parser.problem_mark.line
//                 << ": " << parser.problem << std::endl;
//             return 1;
//         }
// 
//         switch (event.type) {
//             case YAML_STREAM_START_EVENT:
//                 if (event.data.stream_start.encoding != YAML_UTF8_ENCODING) {
//                     std::cerr << "Configuration file not in UTF-8 encoding"
//                         << std::endl;
//                     done = true;
//                     err = 1;
//                 }
//                 break;
// 
//             case YAML_STREAM_END_EVENT:
//                 done = true;
//                 break;
// 
//             case YAML_SEQUENCE_START_EVENT:
//                 sequenceLevel++;
//                 break;
// 
//             case YAML_SEQUENCE_END_EVENT:
//                 sequenceLevel--;
//                 break;
// 
//             case YAML_MAPPING_START_EVENT:
//                 mappingLevel++;
//                 break;
// 
//             case YAML_MAPPING_END_EVENT:
//                 mappingLevel--;
//                 break;
// 
//             case YAML_SCALAR_EVENT:
//                 debug() << std::string((char*)event.data.scalar.value,
//                         event.data.scalar.length) << mappingLevel;
//             case YAML_DOCUMENT_START_EVENT:
//             case YAML_DOCUMENT_END_EVENT:
//             case YAML_NO_EVENT:
//             case YAML_ALIAS_EVENT:
//                 debug() << event.type;
//                 break;
//         }
// 
//         switch (state) {
//             case SECTION_START:
//                 if (event.type == YAML_MAPPING_START_EVENT
//                         and mappingLevel == 1)
//                     state = CHECK_SECTION_NAME;
//                 break;
// 
//             case CHECK_SECTION_NAME:
//                 state = event.type == YAML_SCALAR_EVENT
//                     and !strncmp("lan", (char*)event.data.scalar.value,
//                             event.data.scalar.length)
//                     ? READ_MAPPING : SECTION_START;
// 
//                 break;
// 
//             case READ_MAPPING:
//                 if (event.type == YAML_MAPPING_START_EVENT
//                         and mappingLevel == 2)
//                     state = READ_KEY;
//                 else if (event.type == YAML_MAPPING_END_EVENT
//                         and mappingLevel == 1)
//                     state = SECTION_START;
//                 break;
// 
//             case READ_KEY:
//                 if (event.type == YAML_SCALAR_EVENT) {
//                     key = std::string((char*)event.data.scalar.value,
//                             event.data.scalar.length);
//                     map[key] = std::string();
//                     state = READ_VALUE;
//                 }
//                 else
//                     state = READ_MAPPING;
//                 break;
// 
//             case READ_VALUE:
//                 if (event.type == YAML_SCALAR_EVENT) {
//                     map[key] = std::string((char*)event.data.scalar.value,
//                             event.data.scalar.length);
//                     debug() << key << '=' << map[key];
//                 }
//                 state = READ_MAPPING;
//                 break;
// 
//         }
// 
//         yaml_event_delete(&event);
// 
//     } while(!done);
// 
//     /* END new code */
// 
//     /* Cleanup */
//     yaml_parser_delete(&parser);
//     fclose(fh);
//     return err;
// }
