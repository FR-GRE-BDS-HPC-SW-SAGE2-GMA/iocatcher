######################################################
#            PROJECT  : IO Busters                   #
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
set(IOB_INTERNAL_CODE
	#extern deps
	$<TARGET_OBJECTS:daqpipe-common>
	#internal
#	$<TARGET_OBJECTS:iob-common>
#	$<TARGET_OBJECTS:iob-core>
	#we should keep an eye on portability of this
#	$<TARGET_OBJECTS:iob-portability>
	#optional
#	${OPTIONAL_EXTRA}
)

######################################################
add_library(iobuster SHARED ${IOB_INTERNAL_CODE})
set_target_properties(iobuster PROPERTIES VERSION 0.0.0 SOVERSION 0)
target_link_libraries(iobuster ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(iobuster ${IOB_EXTERN_LIBS})
install(TARGETS iobuster DESTINATION ${CMAKE_INSTALL_LIBDIR})

######################################################
add_subdirectory(common)
