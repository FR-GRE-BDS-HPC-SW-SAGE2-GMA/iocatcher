######################################################
#            PROJECT  : libfabric-poc                #
#            AUTHOR   : Valat Sébastien  - ATOS      #
#            LICENSE  : Apache 2.0                   #
#            COPYRIGHT: 2020 Bull SAS                #
######################################################

######################################################
#Usage of OBJECT keyword in add_library require 2.8.8 (or something like that, need to check)
cmake_minimum_required(VERSION 2.8.8)
# TODO we can remove this when we do not use anymore centos7
if (CMAKE_VERSION VERSION_LESS 3.0)
	project(iocatcher CXX C)
else (CMAKE_VERSION VERSION_LESS 3.0)
	cmake_policy(SET CMP0048 NEW)
	project(iocatcher VERSION 0.0.0 LANGUAGES CXX C)
endif (CMAKE_VERSION VERSION_LESS 3.0)

######################################################
#global setup
set(ENABLE_TESTS ON CACHE BOOL "Enable unit tests requiring build of googletest and googlmock")
if (ENABLE_TESTS)
	enable_testing()
endif(ENABLE_TESTS)

######################################################
#Add internal search path for scripts
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/)

######################################################
#Delegate some cmake scripts
include(${CMAKE_SOURCE_DIR}/cmake/macros.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/unittest-macros.cmake)

######################################################
find_package(LibFabric REQUIRED)
find_package(Threads REQUIRED)
find_package(Libevent REQUIRED)
find_package(LibeventPthreads REQUIRED)
find_package(Mero QUIET)
find_package(Motr QUIET)

######################################################
setup_internal_gmock_and_gtest()
enable_cxx_11()

######################################################
set(ENABLE_COVERAGE OFF CACHE BOOL "Enable coverage profiling.")
if (ENABLE_COVERAGE)
	enable_gcc_coverage()
endif (ENABLE_COVERAGE)

######################################################
include_directories(${LIBFABRIC_INCLUDE_DIRS})
if (MOTR_FOUND)
	include_directories(${MOTR_INCLUDE_DIRS})
	include_directories(${CMAKE_SOURCE_DIR}/extern-deps/from-motr-sample-apps/)
	add_definitions(-DHAVE_MOTR)
	set(MOTR_COMPILE_FLAGS "-D_REENTRANT -D_GNU_SOURCE -DM0_INTERNAL='' -DM0_EXTERN=extern -fno-common -I${MERO_INCLUDE_DIRS}")
	set(MOTR_COMPILE_FLAGS "${MOTR_COMPILE_FLAGS}  -Wall -Wno-attributes -fno-strict-aliasing -rdynamic")
endif (MOTR_FOUND)
if (MERO_FOUND)
	include_directories(${MERO_INCLUDE_DIRS})
	include_directories(${CMAKE_SOURCE_DIR}/extern-deps/from-clovis-sample-apps/)
	add_definitions(-DHAVE_MERO)
	set(MERO_COMPILE_FLAGS "-D_REENTRANT -D_GNU_SOURCE -DM0_INTERNAL='' -DM0_EXTERN=extern -fno-common -I${MERO_INCLUDE_DIRS}")
	set(MERO_COMPILE_FLAGS "${MERO_COMPILE_FLAGS}  -Wall -Wno-attributes -fno-strict-aliasing -rdynamic")
endif(MERO_FOUND)

######################################################
add_executable(poc main.cpp)
target_link_libraries(poc ${LIBFABRIC_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

######################################################
add_definitions(-DENABLE_COLOR)

######################################################
add_subdirectory(extern-deps)
add_subdirectory(src)
