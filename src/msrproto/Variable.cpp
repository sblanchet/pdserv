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

#include "config.h"

#include "Variable.h"
#include "XmlDoc.h"
#include "PrintVariable.h"
#include "../Variable.h"
#include "../Signal.h"
#include "../Parameter.h"
#include "Directory.h"

#include <sstream>

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Variable::Variable( const DirectoryNode *directory,
        const PdServ::Variable *v, unsigned int index,
        unsigned int element, unsigned int nelem):
    directory(directory), index(index), variable(v), nelem(nelem),
    memSize(v->width * nelem), bufferOffset(element * v->width),
    variableElement(element),
    printFunc(getPrintFunc(v->dtype))
{
}

/////////////////////////////////////////////////////////////////////////////
Variable::~Variable()
{
}

/////////////////////////////////////////////////////////////////////////////
std::string Variable::path() const
{
    return directory->path();
}

/////////////////////////////////////////////////////////////////////////////
const PdServ::Signal* Variable::signal() const
{
    return dynamic_cast<const PdServ::Signal*>(variable);
}

/////////////////////////////////////////////////////////////////////////////
const PdServ::Parameter* Variable::parameter() const
{
    return dynamic_cast<const PdServ::Parameter*>(variable);
}
