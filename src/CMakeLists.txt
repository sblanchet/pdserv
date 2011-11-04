#############################################################################
#
#  $Id$
#
#  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
#
#  This file is part of the pdserv package.
#
#  pdserv is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  pdserv is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with pdserv. See COPYING. If not, see
#  <http://www.gnu.org/licenses/>.
#
#############################################################################

INCLUDE_DIRECTORIES(${RTLAB_SOURCE_DIR})
INCLUDE_DIRECTORIES(${LIBCCEXT2_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR})      # for config.h

ADD_LIBRARY( ${PROJECT_NAME} SHARED
    SessionStatistics.h
    Session.cpp Session.h
    Main.cpp Main.h
    Variable.cpp Variable.h
    Signal.cpp Signal.h
    Parameter.cpp Parameter.h

    lib/interface.cpp
    lib/ShmemDataStructures.h
    lib/Task.cpp lib/Task.h
    lib/Main.cpp lib/Main.h
    lib/Signal.cpp lib/Signal.h
    lib/Parameter.cpp lib/Parameter.h

    #etlproto/Server.cpp etlproto/Server.h
    #etlproto/Session.cpp etlproto/Session.h

    msrproto/Subscription.cpp msrproto/Subscription.h
    msrproto/SubscriptionManager.cpp msrproto/SubscriptionManager.h
    msrproto/Outbuf.cpp msrproto/Outbuf.h
    msrproto/Session.cpp msrproto/Session.h
    msrproto/PrintVariable.cpp msrproto/PrintVariable.h
    msrproto/XmlParser.cpp msrproto/XmlParser.h
    msrproto/Attribute.cpp msrproto/Attribute.h
    msrproto/XmlDoc.cpp msrproto/XmlDoc.h
    msrproto/Channel.cpp msrproto/Channel.h
    msrproto/Parameter.cpp msrproto/Parameter.h
    msrproto/Directory.cpp msrproto/Directory.h
    msrproto/Server.cpp msrproto/Server.h
    )

SET_TARGET_PROPERTIES ( ${PROJECT_NAME} PROPERTIES
    VERSION ${VERSION}
    CLEAN_DIRECT_OUTPUT 1)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIBCCEXT2_LIBRARIES})
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION "${ABS_LIBDIR}" LIBRARY)

CONFIGURE_FILE (
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/libpdserv.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/libpdserv.pc" @ONLY
    )
INSTALL (FILES "${CMAKE_CURRENT_BINARY_DIR}/libpdserv.pc" DESTINATION "${ABS_LIBDIR}/pkgconfig")
