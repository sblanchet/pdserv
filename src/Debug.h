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

#ifndef DEBUG_H
#define DEBUG_H

#include "config.h"

#ifdef DEBUG

#include <iostream>

namespace {

struct Debug {
    Debug(const std::string& file, const std::string& func, int line) {
        std::cerr << std::string(file,SRC_PATH_LENGTH)
            << ':' << func << '(' << line << "):";
    }

    ~Debug() {
        std::cerr << std::endl;
    }

    template <class T>
        const Debug& operator<<(const T& o) const {
            std::cerr << ' ' << o;
            return *this;
        }
};

#define debug() Debug(__BASE_FILE__, __func__, __LINE__)
}

#else

namespace {

struct Debug {
    template <class T>
        const Debug& operator<<(const T&) const {
            return *this;
        }
};

#define debug() Debug()
}

#endif

#endif // DEBUG_H
