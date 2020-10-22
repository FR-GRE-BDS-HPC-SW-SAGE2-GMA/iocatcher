######################################################
#            PROJECT  : IO Catcher                   #
#            AUTHOR   : Valat Sébastien  - ATOS      #
#            LICENSE  : Apache 2.0                   #
#            COPYRIGHT: 2020 Bull SAS                #
######################################################

######################################################
set(IOC_SERVER_INTERNAL_CODE
	#extern deps
#	$<TARGET_OBJECTS:daqpipe-common>
	#internal
	$<TARGET_OBJECTS:ioc-server-core>
#	$<TARGET_OBJECTS:ioc-network>
	#we should keep an eye on portability of this
#	$<TARGET_OBJECTS:ioc-portability>
	#optional
#	${OPTIONAL_EXTRA}
)

######################################################
add_library(serverlib SHARED ${IOC_SERVER_INTERNAL_CODE})
target_link_libraries(serverlib iocatcher-base)

######################################################
add_executable(iocatcher-server ${IOC_SERVER_INTERNAL_CODE} main.cpp)
target_link_libraries(iocatcher-server iocatcher-base)
install(TARGETS iocatcher-server DESTINATION ${CMAKE_INSTALL_BINDIR})

######################################################
add_subdirectory(core)