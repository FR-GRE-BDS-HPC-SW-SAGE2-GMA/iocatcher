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
	$<TARGET_OBJECTS:ioc-server-hooks>
#	$<TARGET_OBJECTS:ioc-network>
	#we should keep an eye on portability of this
#	$<TARGET_OBJECTS:ioc-portability>
	#optional
#	${OPTIONAL_EXTRA}
)

######################################################
set(IOC_SERVER_INTERNAL_CODE_NOMERO
	#extern deps
#	$<TARGET_OBJECTS:daqpipe-common>
	#internal
	$<TARGET_OBJECTS:ioc-server-core-no-mero>
	$<TARGET_OBJECTS:ioc-server-hooks>
#	$<TARGET_OBJECTS:ioc-network>
	#we should keep an eye on portability of this
#	$<TARGET_OBJECTS:ioc-portability>
	#optional
#	${OPTIONAL_EXTRA}
)

######################################################
if (MERO_FOUND)
	set(MERO_COMPILE_FLAGS "-D_REENTRANT -D_GNU_SOURCE -DM0_INTERNAL='' -DM0_EXTERN=extern -fno-common -I${MERO_INCLUDE_DIRS}")
	set(MERO_COMPILE_FLAGS "${MERO_COMPILE_FLAGS}  -Wall -Wno-attributes -fno-strict-aliasing -rdynamic")
else (MERO_FOUND)
	set(MERO_COMPILE_FLAGS "-DNOMERO")
endif (MERO_FOUND)

######################################################
if (MERO_FOUND)
	add_library(serverlib SHARED ${IOC_SERVER_INTERNAL_CODE})
	target_link_libraries(serverlib ${MERO_LIBRARIES})
else (MERO_FOUND)
	add_library(serverlib SHARED ${IOC_SERVER_INTERNAL_CODE_NOMERO})
endif (MERO_FOUND)
target_link_libraries(serverlib iocatcher-base)
SET_TARGET_PROPERTIES(serverlib PROPERTIES COMPILE_FLAGS "${MERO_COMPILE_FLAGS}")

######################################################
if (MERO_FOUND)
	add_executable(iocatcher-server ${IOC_SERVER_INTERNAL_CODE} main.cpp)
	target_link_libraries(iocatcher-server iocatcher-base)
	target_link_libraries(iocatcher-server ${MERO_LIBRARIES})
	install(TARGETS iocatcher-server DESTINATION ${CMAKE_INSTALL_BINDIR})
	SET_TARGET_PROPERTIES(iocatcher-server PROPERTIES COMPILE_FLAGS "${MERO_COMPILE_FLAGS}")
endif (MERO_FOUND)

######################################################
add_executable(iocatcher-server-no-mero ${IOC_SERVER_INTERNAL_CODE_NOMERO} main.cpp)
target_compile_definitions(iocatcher-server-no-mero PUBLIC -DNOMERO)
target_link_libraries(iocatcher-server-no-mero iocatcher-base)
install(TARGETS iocatcher-server-no-mero DESTINATION ${CMAKE_INSTALL_BINDIR})

######################################################
add_subdirectory(core)
add_subdirectory(hooks)
