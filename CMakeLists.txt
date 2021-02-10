######################################################
#            PROJECT  : libfabric-poc                #
#            AUTHOR   : Valat Sébastien  - ATOS      #
#            LICENSE  : Apache 2.0                   #
#            COPYRIGHT: 2020 Bull SAS                #
######################################################

######################################################
#Usage of OBJECT keyword in add_library require 2.8.8 (or something like that, need to check)
cmake_minimum_required(VERSION 2.8.8)
project(libfabric-poc)

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
find_package(Mero QUIET)

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
if (MERO_FOUND)
	include_directories(${MERO_INCLUDE_DIRS})
endif(MERO_FOUND)

######################################################
add_executable(poc main.cpp)
target_link_libraries(poc ${LIBFABRIC_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

######################################################
add_definitions(-DENABLE_COLOR)

######################################################
add_subdirectory(extern-deps)
add_subdirectory(src)
