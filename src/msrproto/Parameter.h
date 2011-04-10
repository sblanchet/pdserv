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

#ifndef MSRPARAMETER_H
#define MSRPARAMETER_H

#include <string>
#include <list>
#include <stdint.h>
#include "PrintVariable.h"

namespace PdServ {
    class Parameter;
    class Receiver;
}

namespace MsrXml {
    class Element;
}

namespace MsrProto {

class Session;
class DirectoryNode;

class Parameter {
    public:
        Parameter( const DirectoryNode *directory, bool dependent,
                const PdServ::Parameter *p, unsigned int index,
                unsigned int nelem, unsigned int parameterElement = 0);
        ~Parameter();

        std::string path() const;
//        void addChild(const Parameter *child);

        void setXmlAttributes(MsrXml::Element*, bool shortReply,
                    bool hex, unsigned int flags) const;
        void getValue(char *) const;
        int setHexValue(const char *str, size_t startindex,
                size_t &count) const;
        int setDoubleValue(const char *, size_t startindex,
                size_t &count) const;

        const unsigned int index;
        const PdServ::Parameter * const mainParam;
        const size_t nelem;
        const size_t memSize;
        const size_t bufferOffset;

        const PrintFunc printFunc;

    private:
        const DirectoryNode * const node;
        const bool dependent;

        void (*append)(char *&, double);

        template<class T>
            static void setTo(char *&dst, double src) {
                *reinterpret_cast<T*>(dst) = src;
                dst += sizeof(T);
            }

//        class Converter {
//            public:
//                Converter(Parameter *);
//
//                void setbuf(char *b) const;
//
//                int readDoubleList(const char *val) const;
//                int readHexValue(const char *val) const;
//
//            private:
//                const Parameter * const parameter;
//
//                mutable char * dataBuf;
//
//        };
//
//        Converter converter;

        bool persistent;
};

}

#endif //MSRPARAMETER_H
