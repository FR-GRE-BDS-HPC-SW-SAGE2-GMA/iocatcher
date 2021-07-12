######################################################
# - Try to find libevent (http://directory.fsf.org/wiki/Libfabric)
# Once done this will define
#  LIBEVENT_FOUND - System has libevent
#  LIBEVENT_INCLUDE_DIRS - The libevent include directories
#  LIBEVENT_LIBRARIES - The libraries needed to use libevent
#  LIBEVENT_DEFINITIONS - Compiler switches required for using libevent

######################################################
set(LIBEVENT_PREFIX ${CMAKE_INSTALL_PREFIX} CACHE STRING "Help cmake to find libevent library (https://libevent.org/) into your system.")

######################################################
find_path(LIBEVENT_INCLUDE_DIR event2/event.h
	HINTS ${LIBEVENT_PREFIX}/include)

######################################################
find_library(LIBEVENT_LIBRARY NAMES event
	HINTS ${LIBEVENT_PREFIX}/lib ${LIBEVENT_PREFIX}/lib64)

######################################################
set(LIBEVENT_LIBRARIES ${LIBEVENT_LIBRARY} )
set(LIBEVENT_INCLUDE_DIRS ${LIBEVENT_INCLUDE_DIR} )

######################################################
include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBEVENT_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(Libevent  DEFAULT_MSG
	LIBEVENT_LIBRARY LIBEVENT_INCLUDE_DIR)

######################################################
mark_as_advanced(LIBEVENT_INCLUDE_DIR LIBEVENT_LIBRARY )