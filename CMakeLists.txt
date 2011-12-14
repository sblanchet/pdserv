#############################################################################
#
#  $Id$
#
#  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
#
#  This is the main cmake file for building process data communications server
#
#  The following options can be specified on the command line of cmake to
#  change the installation paths (Defaults are in <>)
#       -DPREFIX=</opt/etherlab>
#       -DINCLUDEDIR=<${prefix}/include>
#       -DEXECDIR=<${prefix}/bin>
#       -DLIBDIR=<${prefix}/lib>
#       -DTRADITIONAL=0           # Traditional mode for MSR
#       -DDISABLE_BUDDY=0         # Do not build etherlab_buddy
#
#  For debugging, use the
#       -DCMAKE_BUILD_TYPE=Debug|Release|RelWithDebInfo|MinSizeRel
#  option
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


CMAKE_MINIMUM_REQUIRED (VERSION 2.6)
PROJECT (pdserv)
SET (CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS TRUE)

# Project version
SET (MAJOR_VERSION 0)
SET (MINOR_VERSION 99)
SET (PATCH_LEVEL 0)
SET (VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_LEVEL})
#SET (${PROJECT_NAME}_SOVERSION "SOVERSION")
#    ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_LEVEL})

# Project path options
IF (NOT PREFIX)         # Project root
    SET (PREFIX "/opt/etherlab")
ENDIF ()

SET (CMAKE_INSTALL_PREFIX "${PREFIX}")
LIST (APPEND CMAKE_PREFIX_PATH "${PREFIX}")

IF (SYSCONFDIR)         # configuration files
    SET (ABS_SYSCONFDIR "${SYSCONFDIR}")
    SET (PC_SYSCONFDIR "${SYSCONFDIR}")
ELSE ()
    SET (ABS_SYSCONFDIR ${PREFIX}/etc)
    SET (PC_SYSCONFDIR "\${prefix}/etc")
ENDIF ()

IF (INCLUDEDIR)         # include files
    SET (ABS_INCLUDEDIR "${INCLUDEDIR}")
    SET (PC_INCLUDEDIR "${INCLUDEDIR}")
    LIST (APPEND CMAKE_INCLUDE_PATH "${INCLUDEDIR}")
ELSE ()
    SET (ABS_INCLUDEDIR ${PREFIX}/include)
    SET (PC_INCLUDEDIR "\${prefix}/include")
ENDIF ()

IF (EXECDIR)            # executables
    SET (ABS_EXECDIR "${EXECDIR}")
    SET (PC_EXECDIR "${EXECDIR}")
ELSE ()
    SET (ABS_EXECDIR ${PREFIX}/bin)
    SET (PC_EXECDIR "\${prefix}/bin")
ENDIF ()

IF (LIBDIR)             # libraries
    SET (ABS_LIBDIR "${LIBDIR}")
    SET (PC_LIBDIR "${LIBDIR}")
ELSE ()
    SET (ABS_LIBDIR ${PREFIX}/lib)
    SET (PC_LIBDIR "\${prefix}/lib")
ENDIF ()

# Make sure that the project options are remembered
SET (PREFIX "${PREFIX}" CACHE PATH
    "Root directory for the project" FORCE)
SET (SYSCONFDIR "${SYSCONFDIR}" CACHE PATH
    "Directory for configuration files" FORCE)
SET (INCLUDEDIR "${INCLUDEDIR}" CACHE PATH
    "Directory for header files" FORCE)
SET (EXECDIR "${EXECDIR}" CACHE PATH
    "Directory for executables" FORCE)
SET (LIBDIR "${LIBDIR}" CACHE PATH
    "Directory for libraries" FORCE)

SET (DISABLE_BUDDY "${DISABLE_BUDDY}" CACHE BOOL
    "Do not build etherlab_buddy" FORCE)

SET (TRADITIONAL "${TRADITIONAL}" CACHE BOOL
    "Traditional mode for MSR" FORCE)

ENABLE_TESTING()

INCLUDE (InstallRequiredSystemLibraries)

SET (CPACK_PACKAGE_DESCRIPTION_SUMMARY "Process data communications library")
SET (CPACK_PACKAGE_VENDOR "Richard Hacker")
SET (CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.txt")
SET (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
SET (CPACK_PACKAGE_VERSION_MAJOR ${MAJOR_VERSION})
SET (CPACK_PACKAGE_VERSION_MINOR ${MINOR_VERSION})
SET (CPACK_PACKAGE_VERSION_PATCH ${PATCH_LEVEL})
SET (CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")
SET (CPACK_SOURCE_GENERATOR "TBZ2")
SET (CPACK_SOURCE_PACKAGE_FILE_NAME
      "${PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
SET (CPACK_GENERATOR "TGZ")
SET (CPACK_GENERATOR "DEB")
SET (CPACK_DEBIAN_PACKAGE_MAINTAINER "Richard Hacker") #required
SET (CPACK_SOURCE_IGNORE_FILES
      "/.hg;~$;.sw.$;${CPACK_SOURCE_IGNORE_FILES}")

INCLUDE (CPack)
ADD_CUSTOM_TARGET (dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

# Search for mercurial, used to add ID information to the library
SET (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
FIND_PACKAGE (Mercurial REQUIRED)
MERCURIAL_HG_INFO (${PROJECT_SOURCE_DIR} Package)

# Require CommonC++
FIND_PACKAGE (PkgConfig REQUIRED)
PKG_CHECK_MODULES (LIBCCEXT2 REQUIRED libccext2)

IF (UNIX)
    ADD_CUSTOM_TARGET (tags etags -R ${CMAKE_CURRENT_SOURCE_DIR})
    ADD_CUSTOM_TARGET (etags DEPENDS tags)
ENDIF ()

ADD_DEFINITIONS ("-Wall -ansi")

INCLUDE_DIRECTORIES("${PROJECT_SOURCE_DIR}/include" "${ABS_INCLUDEDIR}")
ADD_SUBDIRECTORY (src)
ADD_SUBDIRECTORY (test)

# include (ExternalProject)
# ExternalProject_Add(
#     iniparser
#     URL ${CMAKE_CURRENT_SOURCE_DIR}/iniparser-3.0.tar.gz
#     CONFIGURE_COMMAND ""
#     BUILD_IN_SOURCE 1
#     INSTALL_COMMAND ""
# )

INSTALL (DIRECTORY include/
    DESTINATION "${ABS_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h")

OPTION ( DEBUG "Debugging output" OFF )

if (DEBUG)
    string(LENGTH "${PROJECT_SOURCE_DIR}/src/" SRC_PATH_LENGTH)
endif (DEBUG)

CONFIGURE_FILE (
    "${PROJECT_SOURCE_DIR}/config.h.in"
    "${PROJECT_BINARY_DIR}/config.h"
    )

