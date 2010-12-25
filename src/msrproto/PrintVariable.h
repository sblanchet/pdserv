/*****************************************************************************
 *
 *  $Id$
 *
 *  This is a minimal implementation of the XML dialect as used by
 *  the MSR protocol.
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

#ifndef PRINTVARIABLE_H
#define PRINTVARIABLE_H

#include <sstream>
#include <string>
#include "pdcomserv/etl_data_info.h"

namespace HRTLab {
    class Variable;
}

namespace MsrXml {
    class Element;
}

namespace MsrProto {

typedef void (*PrintFunc)(std::ostringstream&,
        const char *, size_t, unsigned int);
PrintFunc getPrintFunc(enum si_datatype_t dtype);

void base64Attribute(MsrXml::Element *element, const char *name,
        const HRTLab::Variable *v, size_t count, const char *data);
void hexDecAttribute(MsrXml::Element *element, const char *name,
        const HRTLab::Variable *v, size_t count, const char *data);

void csvAttribute(MsrXml::Element *element, const char *name,
        PrintFunc printfunc, const HRTLab::Variable *v,
        size_t count, const char *data, size_t precision = 16);

std::string toCSV( PrintFunc printfunc, const HRTLab::Variable *v,
        size_t count, const char *data, size_t precision = 16);

/** Special functions to set Parameter and Channel
 * attributes */
void setVariableAttributes( MsrXml::Element *element,
        const HRTLab::Variable*, unsigned int index,
        const std::string& path, size_t nelem, bool shortReply);

}

#endif // PRINTVARIABLE_H
