/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 - 2012  Richard Hacker (lerichi at gmx dot net)
 *                         Florian Pose <fp@igh-essen.com>
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

#include "../Debug.h"
#include "SessionData.h"
#include "ShmemDataStructures.h"
#include "Event.h"

////////////////////////////////////////////////////////////////////////////
SessionData::SessionData (EventData* rp): readPointer(rp)
{
}