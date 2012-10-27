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

#ifndef CONFIGFILE_H
#define CONFIGFILE_H

#include <stdexcept>
#include <string>
#include <yaml.h>

namespace PdServ {

class Config {
    public:
        Config();
        ~Config();

        const char * load(const char *file);
        const std::string& fileName() const;

        Config operator[](const std::string&) const;
        Config operator[](const char *) const;
        Config operator[](char *) const;
        Config operator[](size_t) const;

        template <typename T>
            bool get(T& value) const;

        operator bool() const;

        int             toInt() const;
        unsigned int    toUInt() const;
        double          toDouble() const;
        std::string     toString() const;

    protected:
        Config(yaml_document_t *document, yaml_node_t *node);

    private:
        std::string file;

        yaml_document_t *document;
        yaml_node_t *node;

        void clear();
};

inline Config Config::operator[](char *key) const {
    return operator[](static_cast<const char *>(key));
}

};
#endif // CONFIGFILE_H
