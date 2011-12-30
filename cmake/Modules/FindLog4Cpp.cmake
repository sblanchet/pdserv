# - Try to find Log4Cpp
# Once done this will define
#  LOG4CPP_FOUND - System has log4cpp
#  LOG4CPP_INCLUDE_DIRS - The log4cpp include directories
#  LOG4CPP_LIBRARIES - The libraries needed to use log4cpp
#  LOG4CPP_DEFINITIONS - Compiler switches required for using log4cpp

find_package(PkgConfig)
pkg_check_modules(PC_LOG4CPP log4cpp)
set(LOG4CPP_DEFINITIONS ${PC_LOG4CPP_CFLAGS_OTHER})

find_path(LOG4CPP_INCLUDE_DIR log4cpp/SimpleLayout.hh
        HINTS ${PC_LOG4CPP_INCLUDEDIR} ${PC_LOG4CPP_INCLUDE_DIRS}
        PATH_SUFFIXES log4cpp )

find_library(LOG4CPP_LIBRARY
        NAMES ${PC_LOG4CPP_LIBRARIES} log4cpp 
        HINTS ${PC_LOG4CPP_LIBDIR} ${PC_LOG4CPP_LIBRARY_DIRS} )

set(LOG4CPP_LIBRARIES ${LOG4CPP_LIBRARY} )
set(LOG4CPP_INCLUDE_DIRS ${LOG4CPP_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LOG4CPP_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(Log4Cpp  DEFAULT_MSG
        LOG4CPP_LIBRARY LOG4CPP_INCLUDE_DIR)

mark_as_advanced(LOG4CPP_INCLUDE_DIR LOG4CPP_LIBRARY )
