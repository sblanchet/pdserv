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

#ifndef BUDDYCONFIG_H
#define BUDDYCONFIG_H

#include <map>
#include <yaml.h>
#include "../ServerConfig.h"

class BuddyConfigRef: public PdServ::ServerConfig {
    public:
        BuddyConfigRef(yaml_document_t * = 0, yaml_node_t * = 0);

        bool get(const std::string *name, std::string &value);

    private:
        yaml_node_t * const node;
        yaml_document_t * const document;
};

class BuddyConfig {
    public:
        BuddyConfig();
        ~BuddyConfig();

        BuddyConfigRef select(const char *map);
        int load_seq(const std::string &file);
        int load(const std::string &file);

    private:
        yaml_node_t *root;
        yaml_document_t *document;
};

#endif // BUDDYCONFIG_H
