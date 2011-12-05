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

#ifndef BUDDY_SIGNALINFO
#define BUDDY_SIGNALINFO

#include "../Variable.h"
#include <etl_data_info.h>

class SignalInfo {
    public:
        SignalInfo( const char *model, const struct signal_info *si);

        std::string path() const;
        size_t ndim() const;
        const size_t *dim() const;
        PdServ::Variable::Datatype dataType() const;

        void read( void *dst, const void *src) const;
        void write( void *dst, const void *src,
                size_t start, size_t count) const;

        const struct signal_info * const si;

    private:
        const char * const model;

        size_t _dim[2];
        size_t nelem;
        size_t rows;

        typedef void (*CopyFunc)(void *dst, const void *src,
                size_t start, size_t count, size_t nelem, size_t cols);
        CopyFunc readFunc;
        CopyFunc writeFunc;


        template <class T, char dir>
            static void copy( void *dst, const void *src,
                    size_t start, size_t count, size_t nelem, size_t cols);

        template <class T, char dir>
            static void transpose( void *dst, const void *src,
                    size_t start, size_t count, size_t nelem, size_t cols);
};

#endif //BUDDY_SIGNALINFO
