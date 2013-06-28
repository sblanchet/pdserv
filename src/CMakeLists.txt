#############################################################################
#
#  $Id$
#
#  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
#
#  This file is part of the pdserv library.
#
#  The pdserv library is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or (at your
#  option) any later version.
#
#  The pdserv library is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
#  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
#  License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with the pdserv library. If not, see <http://www.gnu.org/licenses/>.
#
#############################################################################

INCLUDE_DIRECTORIES(
        ${PROJECT_BINARY_DIR}       # for config.h
        ${LIBCCEXT2_INCLUDE_DIRS}
        )

SET (CMAKE_CXX_FLAGS "-Wunused")

SET (msrproto_src
    msrproto/StatSignal.cpp             msrproto/StatSignal.h
    msrproto/Event.cpp                  msrproto/Event.h
    msrproto/TimeSignal.cpp             msrproto/TimeSignal.h
    msrproto/Subscription.cpp           msrproto/Subscription.h
    msrproto/SubscriptionManager.cpp    msrproto/SubscriptionManager.h
    msrproto/Session.cpp                msrproto/Session.h
    msrproto/XmlParser.cpp              msrproto/XmlParser.h
    msrproto/Attribute.cpp              msrproto/Attribute.h
    msrproto/XmlElement.cpp             msrproto/XmlElement.h
    msrproto/Variable.cpp               msrproto/Variable.h
    msrproto/Channel.cpp                msrproto/Channel.h
    msrproto/Parameter.cpp              msrproto/Parameter.h
    msrproto/DirectoryNode.cpp          msrproto/DirectoryNode.h
    msrproto/HyperDirNode.cpp           msrproto/HyperDirNode.h
    msrproto/Server.cpp                 msrproto/Server.h
    )

SET (main_src
                        SessionStatistics.h
    Config.cpp          Config.h
    Session.cpp         Session.h
    SessionTask.cpp     SessionTask.h
    Task.cpp            Task.h
    Main.cpp            Main.h
    Variable.cpp        Variable.h
    DataType.cpp        DataType.h
    Signal.cpp          Signal.h
    Event.cpp           Event.h
    Parameter.cpp       Parameter.h
    ProcessParameter.cpp ProcessParameter.h
    Debug.cpp           Debug.h

    supervisor/Server.cpp         supervisor/Server.h
    )

# Search for files required by buddy.
FIND_PATH (RT_APP_H "rt_app.h" PATHS /opt/etherlab/include)

#MESSAGE("ETL_DATA=${RT_APP_H}")

IF (RT_APP_H AND ENABLE_BUDDY)

    INCLUDE_DIRECTORIES(${RT_APP_H})

    SET (TESTFILE "${CMAKE_CURRENT_SOURCE_DIR}/test_headers.c")

    TRY_COMPILE (HAVE_TIMESPEC "${CMAKE_BINARY_DIR}" "${TESTFILE}"
        CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${RT_APP_H}"
        COMPILE_DEFINITIONS "-DHAVE_TIMESPEC"
        OUTPUT_VARIABLE HAVE_TIMESPEC_OUTPUT)

    TRY_COMPILE (HAVE_TIMEVAL "${CMAKE_BINARY_DIR}" "${TESTFILE}"
        CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${RT_APP_H}"
        COMPILE_DEFINITIONS "-DHAVE_TIMEVAL")

    IF (HAVE_TIMEVAL)
        MESSAGE (STATUS "Using timeval for etherlab_buddy2")
    ELSEIF (HAVE_TIMESPEC)
        MESSAGE (STATUS "Using timespec for etherlab_buddy2")
        SET_SOURCE_FILES_PROPERTIES (buddy/SessionTaskData.cpp
            PROPERTIES COMPILE_DEFINITIONS HAVE_TIMESPEC)
    ELSE ()
        MESSAGE ("Compiler output:")
        MESSAGE (${HAVE_TIMESPEC_OUTPUT})
        MESSAGE (FATAL_ERROR
            "Not all header files for etherlab_buddy are available")
    ENDIF()

    TRY_COMPILE (HAVE_SIMULINK_PORT "${CMAKE_BINARY_DIR}" "${TESTFILE}"
        CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${RT_APP_H}"
        COMPILE_DEFINITIONS "-DHAVE_PORT"
        OUTPUT_VARIABLE V2)

    IF (HAVE_SIMULINK_PORT)
        MESSAGE (STATUS "etherlab_buddy2 understands port= option")
        SET_SOURCE_FILES_PROPERTIES (buddy/SignalInfo.cpp
            PROPERTIES COMPILE_DEFINITIONS HAVE_SIMULINK_PORT)
    ENDIF ()

    #MESSAGE("${HAVE_TIMESPEC_OUTPUT}")
    #MESSAGE("${V2}")
    #MESSAGE("${CMAKE_INCLUDE_PATH}")
    #MESSAGE("TIME=${HAVE_TIMESPEC} ${HAVE_SIMULINK_PORT}")

    FIND_PACKAGE (LibDaemon REQUIRED)
    FIND_PACKAGE (Threads REQUIRED)

    ADD_EXECUTABLE (etherlab_buddy2
        buddy/SignalInfo.cpp            buddy/SignalInfo.h
        buddy/Signal.cpp                buddy/Signal.h
        buddy/Event.cpp                 buddy/Event.h
        buddy/EventQ.cpp                buddy/EventQ.h
        buddy/Task.cpp                  buddy/Task.h
        buddy/Main.cpp                  buddy/Main.h
        buddy/Parameter.cpp             buddy/Parameter.h
        buddy/SessionTaskData.cpp       buddy/SessionTaskData.h
        buddy/main.cpp

        "${ETL_DATA}" "${BUDDY_IOCTL}"

        ${msrproto_src}
        ${main_src}
        )

    SET (BUDDY_LIBS
        ${LIBCCEXT2_LIBRARIES}
        ${LIBDAEMON_LIBRARIES}
        ${YAML_LIBRARIES}
        ${LOG4CPLUS_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        )

    INCLUDE_DIRECTORIES (${LIBDAEMON_INCLUDE_DIRS})
    TARGET_LINK_LIBRARIES (etherlab_buddy2 ${BUDDY_LIBS})
    INSTALL (TARGETS etherlab_buddy2
        DESTINATION "${CMAKE_INSTALL_BINDIR}" RUNTIME)

    INSTALL (FILES "${CMAKE_CURRENT_SOURCE_DIR}/buddy/buddy.conf"
        DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}")
ENDIF ()

ADD_LIBRARY( ${PROJECT_NAME} SHARED
    lib/interface.cpp
                            lib/ShmemDataStructures.h
    lib/Task.cpp            lib/Task.h
    lib/Main.cpp            lib/Main.h
    lib/Signal.cpp          lib/Signal.h
    lib/Event.cpp           lib/Event.h
    lib/Parameter.cpp       lib/Parameter.h
    lib/SessionTaskData.cpp lib/SessionTaskData.h
    lib/SessionData.cpp     lib/SessionData.h

    ${msrproto_src}
    ${main_src}
    )

SET_TARGET_PROPERTIES ( ${PROJECT_NAME} PROPERTIES
    VERSION ${VERSION}
    SOVERSION "${SOVERSION}"
    CLEAN_DIRECT_OUTPUT 1)

TARGET_LINK_LIBRARIES (${PROJECT_NAME} ${LIBCCEXT2_LDFLAGS}
    ${YAML_LIBRARIES} ${LOG4CPLUS_LIBRARIES})
INSTALL (TARGETS ${PROJECT_NAME} DESTINATION "${CMAKE_INSTALL_LIBDIR}" LIBRARY)

# pkgconfig files
CONFIGURE_FILE (
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/libpdserv.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/libpdserv.pc" @ONLY
    )
INSTALL (FILES "${CMAKE_CURRENT_BINARY_DIR}/libpdserv.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
