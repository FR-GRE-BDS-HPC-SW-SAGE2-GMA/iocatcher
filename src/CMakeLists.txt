######################################################
#            PROJECT  : IO Catcher                   #
#            AUTHOR   : Valat Sébastien  - ATOS      #
#            LICENSE  : Apache 2.0                   #
#            COPYRIGHT: 2020 Bull SAS                #
######################################################

######################################################
#vars
set(WITH_OS "Unix" CACHE STRING "On which OS we are running on to adapt system functions and rules. Currently only support 'Unix")

######################################################
#setup config file
configure_file(config.h.in "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

######################################################
include_directories(${CMAKE_SOURCE_DIR}/extern-deps)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

######################################################
add_subdirectory(base)
add_subdirectory(client)
add_subdirectory(server)

######################################################
if (ENABLE_TESTS)
	add_subdirectory(tests)
endif(ENABLE_TESTS)
