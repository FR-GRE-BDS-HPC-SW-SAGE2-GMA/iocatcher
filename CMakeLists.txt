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
enable_testing()

######################################################
#Add internal search path for scripts
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/)

######################################################
#Delegate some cmake scripts
include(${CMAKE_SOURCE_DIR}/cmake/macros.cmake)

######################################################
find_package(LibFabric REQUIRED)
find_package(Threads REQUIRED)

######################################################
enable_cxx_11()

######################################################
include_directories(${LIBFABRIC_INCLUDE_DIRS})
add_executable(poc main.cpp)
target_link_libraries(poc ${LIBFABRIC_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})