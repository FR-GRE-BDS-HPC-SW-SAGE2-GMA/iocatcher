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
set(MERO_COMPILE_FLAGS "-D_REENTRANT -D_GNU_SOURCE -DM0_INTERNAL='' -DM0_EXTERN=extern -fno-common -I${MERO_INCLUDE_DIRS}")
set(MERO_COMPILE_FLAGS "${MERO_COMPILE_FLAGS}  -Wall -Wno-attributes -fno-strict-aliasing -rdynamic")

######################################################
add_library(serverlib SHARED ${IOC_SERVER_INTERNAL_CODE})
target_link_libraries(serverlib iocatcher-base)
target_link_libraries(serverlib ${MERO_LIBRARIES})
SET_TARGET_PROPERTIES(serverlib PROPERTIES COMPILE_FLAGS "${MERO_COMPILE_FLAGS}")

######################################################
add_executable(iocatcher-server ${IOC_SERVER_INTERNAL_CODE} main.cpp)
target_link_libraries(iocatcher-server iocatcher-base)
target_link_libraries(iocatcher-server ${MERO_LIBRARIES})
install(TARGETS iocatcher-server DESTINATION ${CMAKE_INSTALL_BINDIR})
SET_TARGET_PROPERTIES(iocatcher-server PROPERTIES COMPILE_FLAGS "${MERO_COMPILE_FLAGS}")

######################################################
add_subdirectory(core)