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

INCLUDE_DIRECTORIES(
        ${PROJECT_BINARY_DIR}       # for config.h
        ${RTLAB_SOURCE_DIR}
        ${LIBCCEXT2_INCLUDE_DIRS}
        )

SET (msrproto_src
    msrproto/StatSignal.cpp             msrproto/StatSignal.h
    msrproto/TimeSignal.cpp             msrproto/TimeSignal.h
    msrproto/Subscription.cpp           msrproto/Subscription.h
    msrproto/SubscriptionManager.cpp    msrproto/SubscriptionManager.h
    #msrproto/Outbuf.cpp                 msrproto/Outbuf.h
    msrproto/Session.cpp                msrproto/Session.h
    msrproto/XmlParser.cpp              msrproto/XmlParser.h
    msrproto/Attribute.cpp              msrproto/Attribute.h
    msrproto/XmlElement.cpp             msrproto/XmlElement.h
    msrproto/Variable.cpp               msrproto/Variable.h
    msrproto/Channel.cpp                msrproto/Channel.h
    msrproto/Parameter.cpp              msrproto/Parameter.h
    msrproto/Directory.cpp              msrproto/Directory.h
    msrproto/Server.cpp                 msrproto/Server.h
    )

SET (main_src
                        SessionStatistics.h
    Config.cpp          Config.h
    Session.cpp         Session.h
    Task.cpp            Task.h
    Main.cpp            Main.h
    Variable.cpp        Variable.h
    Signal.cpp          Signal.h
    Parameter.cpp       Parameter.h
                        SessionShadow.h
    Debug.cpp           Debug.h
    SessionTaskData.cpp SessionTaskData.h
    )

# # Search for files required by buddy.
# SET (CMAKE_REQUIRED_INCLUDES ${ABS_INCLUDEDIR})
# INCLUDE( CheckIncludeFile )
# CHECK_INCLUDE_FILE(etl_data_info.h ETL_DATA)
# CHECK_INCLUDE_FILE(fio_ioctl.h BUDDY_IOCTL)

FIND_PATH (YAML_INCLUDE yaml.h)
FIND_LIBRARY (YAML_LIBS yaml)
IF (NOT YAML_LIBS OR NOT YAML_INCLUDE)
    MESSAGE (FATAL_ERROR "Cannot find yaml development libraries")
ENDIF ()
INCLUDE_DIRECTORIES (${YAML_INCLUDE})

# Search for files required by buddy.
FIND_FILE (ETL_DATA "etl_data_info.h")
FIND_FILE (BUDDY_IOCTL "fio_ioctl.h")

#MESSAGE("ETL_DATA=${ETL_DATA} BUDDY_IOCTL=${BUDDY_IOCTL}")

IF (ETL_DATA AND BUDDY_IOCTL AND NOT DISABLE_BUDDY)

    SET (TESTFILE "${CMAKE_CURRENT_SOURCE_DIR}/test_headers.c")

    TRY_COMPILE (HAVE_TIMESPEC "${CMAKE_BINARY_DIR}" "${TESTFILE}"
            CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${ABS_INCLUDEDIR}"
            COMPILE_DEFINITIONS "-DHAVE_TIMESPEC")

    TRY_COMPILE (HAVE_SIMULINK_PORT "${CMAKE_BINARY_DIR}" "${TESTFILE}"
            CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${ABS_INCLUDEDIR}"
            COMPILE_DEFINITIONS "-DHAVE_PORT")

    IF (HAVE_TIMESPEC)
        set_source_files_properties (buddy/SessionTaskData.cpp
            PROPERTIES COMPILE_DEFINITIONS HAVE_TIMESPEC)
    ENDIF()

    IF (HAVE_SIMULINK_PORT)
        set_source_files_properties (buddy/SignalInfo.cpp
            PROPERTIES COMPILE_DEFINITIONS HAVE_SIMULINK_PORT)
    ENDIF ()

    #MESSAGE("TIME=${HAVE_TIMESPEC} ${HAVE_SIMULINK_PORT}")

    FIND_PACKAGE (LibDaemon REQUIRED)

    ADD_EXECUTABLE (etherlab_buddy2
        buddy/SignalInfo.cpp buddy/SignalInfo.h
        buddy/Signal.cpp buddy/Signal.h
        buddy/main.cpp
        buddy/Task.cpp buddy/Task.h
        buddy/Main.cpp buddy/Main.h
        buddy/Parameter.cpp buddy/Parameter.h
        buddy/SessionShadow.cpp buddy/SessionShadow.h
        buddy/SessionTaskData.cpp buddy/SessionTaskData.h

        "${ETL_DATA}" "${BUDDY_IOCTL}"

        ${msrproto_src}
        ${main_src}
        )

    SET (BUDDY_LIBS
        ${LIBCCEXT2_LIBRARIES}
        ${LIBDAEMON_LIBRARIES}
        ${YAML_LIBS}
        )

    INCLUDE_DIRECTORIES (${LIBDAEMON_INCLUDE_DIRS})
    TARGET_LINK_LIBRARIES (etherlab_buddy2 ${BUDDY_LIBS})
    INSTALL (TARGETS etherlab_buddy2 DESTINATION "${ABS_EXECDIR}" RUNTIME)

ENDIF ()

ADD_LIBRARY( ${PROJECT_NAME} SHARED
    lib/interface.cpp
                            lib/ShmemDataStructures.h
    lib/Task.cpp            lib/Task.h
    lib/Main.cpp            lib/Main.h
    lib/Signal.cpp          lib/Signal.h
    lib/Parameter.cpp       lib/Parameter.h
    lib/SessionShadow.cpp   lib/SessionShadow.h
    lib/SessionTaskData.cpp lib/SessionTaskData.h

    ${msrproto_src}
    ${main_src}
    )

SET_TARGET_PROPERTIES ( ${PROJECT_NAME} PROPERTIES
    VERSION ${VERSION}
    CLEAN_DIRECT_OUTPUT 1)

TARGET_LINK_LIBRARIES (${PROJECT_NAME} ${LIBCCEXT2_LIBRARIES} ${YAML_LIBS})
INSTALL (TARGETS ${PROJECT_NAME} DESTINATION "${ABS_LIBDIR}" LIBRARY)

# pkgconfig files
CONFIGURE_FILE (
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/libpdserv.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/libpdserv.pc" @ONLY
    )
INSTALL (FILES "${CMAKE_CURRENT_BINARY_DIR}/libpdserv.pc"
    DESTINATION "${ABS_LIBDIR}/pkgconfig")
