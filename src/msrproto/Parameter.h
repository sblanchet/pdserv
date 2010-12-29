/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdcomserv package.
 *
 *  pdcomserv is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pdcomserv is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pdcomserv. See COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#ifndef MSRPARAMETER_H
#define MSRPARAMETER_H

#include <string>
#include <list>
#include <stdint.h>
#include "PrintVariable.h"

namespace HRTLab {
    class Parameter;
    class Receiver;
}

namespace MsrXml {
    class Element;
}

namespace MsrProto {

class Session;

class Parameter {
    public:
        Parameter( const HRTLab::Parameter *s, unsigned int index);
        Parameter( const HRTLab::Parameter *s, unsigned int index,
                unsigned int paramOffset, size_t nelem = 1);
        ~Parameter();

        std::string path() const;
        void setXmlAttributes(MsrXml::Element*, bool shortReply,
                    bool hex, unsigned int flags) const;
        void getValue(char *) const;
        int setHexValue(const char *str, size_t startindex) const;
        int setDoubleValue(const char *, size_t startindex) const;

        const unsigned int index;
        const HRTLab::Parameter * const mainParam;
        const size_t nelem;
        const size_t memSize;
        const size_t bufferOffset;

        const PrintFunc printFunc;

    private:

        class Converter {
            public:
                Converter(Parameter *);

                void setbuf(char *b) const;

                int readDoubleList(const char *val, size_t startindex) const;
                int readHexValue(const char *val, size_t startindex) const;

            private:
                const Parameter * const parameter;

                mutable char * dataBuf;
                void (*append)(char *&, double);

                template<class T>
                static void setTo(char *&dst, double src) {
                    *reinterpret_cast<T*>(dst) = src;
                    dst += sizeof(T);
                }
        };

        Converter converter;

        bool persistent;

        std::string extension;

};

}

#endif //MSRPARAMETER_H
