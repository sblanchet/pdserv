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

#ifndef CONFIGFILE_H
#define CONFIGFILE_H

#include <stdexcept>
#include <string>

namespace PdServ {

// struct ConfigEmpty: public std::runtime_error {
//     ConfigEmpty();
// };
// 
// struct ConfigError: public std::runtime_error {
//     ConfigError(yaml_document_t *document, yaml_node_t *node,
//             const std::string& reason);
// };

class Config {
    public:
        Config();
        Config(const Config& other);
        ~Config();

        const char * load(const char *file);

        Config operator[](const std::string&) const;
        Config operator[](const char *) const;
        Config operator[](size_t) const;

        operator int() const;
        operator unsigned int() const;
        operator double() const;
        operator std::string() const;

    protected:
        Config(yaml_document_t *document, yaml_node_t *node);

    private:
        std::string file;

        yaml_document_t *document;
        yaml_node_t *node;
};

};
#endif // CONFIGFILE_H