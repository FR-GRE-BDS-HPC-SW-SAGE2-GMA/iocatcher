######################################################
# - Try to find libevent (http://directory.fsf.org/wiki/Libfabric)
# Once done this will define
#  LIBEVENT_PTHREADS_FOUND - System has libevent
#  LIBEVENT_PTHREADS_INCLUDE_DIRS - The libevent include directories
#  LIBEVENT_PTHREADS_LIBRARIES - The libraries needed to use libevent
#  LIBEVENT_PTHREADS_DEFINITIONS - Compiler switches required for using libevent

######################################################
set(LIBEVENT_PTHREADS_PREFIX "" CACHE STRING "Help cmake to find libevent library (https://libevent.org/) into your system.")

######################################################
find_path(LIBEVENT_PTHREADS_INCLUDE_DIR event2/thread.h
	HINTS ${LIBEVENT_PTHREADS_PREFIX}/include)

######################################################
find_library(LIBEVENT_PTHREADS_LIBRARY NAMES event_pthreads
	HINTS ${LIBEVENT_PTHREADS_PREFIX}/lib ${LIBEVENT_PTHREADS_PREFIX}/lib64)

######################################################
set(LIBEVENT_PTHREADS_LIBRARIES ${LIBEVENT_PTHREADS_LIBRARY} )
set(LIBEVENT_PTHREADS_INCLUDE_DIRS ${LIBEVENT_PTHREADS_INCLUDE_DIR} )

######################################################
include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBEVENT_PTHREADS_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(libeventpthreads  DEFAULT_MSG
	LIBEVENT_PTHREADS_LIBRARY LIBEVENT_PTHREADS_INCLUDE_DIR)

######################################################
mark_as_advanced(LIBEVENT_PTHREADS_INCLUDE_DIR LIBEVENT_PTHREADS_LIBRARY )