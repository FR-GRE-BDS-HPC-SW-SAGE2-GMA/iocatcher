######################################################
#  PROJECT  : IO Catcher                             #
#  LICENSE  : Apache 2.0                             #
#  COPYRIGHT: 2020-2022 Bull SAS All rights reserved #
######################################################

######################################################
set(IOC_SERVER_INTERNAL_CODE
	#extern deps
#	$<TARGET_OBJECTS:daqpipe-common>
	#internal
	$<TARGET_OBJECTS:ioc-server-core>
	$<TARGET_OBJECTS:ioc-server-hooks>
	$<TARGET_OBJECTS:ioc-server-backends>
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
	$<TARGET_OBJECTS:ioc-server-core>
	$<TARGET_OBJECTS:ioc-server-hooks>
	$<TARGET_OBJECTS:ioc-server-backends-no-mero>
#	$<TARGET_OBJECTS:ioc-network>
	#we should keep an eye on portability of this
#	$<TARGET_OBJECTS:ioc-portability>
	#optional
#	${OPTIONAL_EXTRA}
)

######################################################
if (MERO_FOUND)
	list(APPEND IOC_SERVER_INTERNAL_CODE $<TARGET_OBJECTS:ioc-extern-mero>)
endif(MERO_FOUND)
if (MOTR_FOUND)
	list(APPEND IOC_SERVER_INTERNAL_CODE $<TARGET_OBJECTS:ioc-extern-motr>)
endif (MOTR_FOUND)
if (NOT MERO_FOUND AND NOT MOTR_FOUND)
	set(MERO_COMPILE_FLAGS "-DNOMERO")
endif (NOT MERO_FOUND AND NOT MOTR_FOUND)

######################################################
if (MERO_FOUND)
	add_library(serverlib SHARED ${IOC_SERVER_INTERNAL_CODE})
	target_link_libraries(serverlib ${MERO_LIBRARIES})
	SET_TARGET_PROPERTIES(serverlib PROPERTIES COMPILE_FLAGS "${MERO_COMPILE_FLAGS}")
elseif (MOTR_FOUND)
	add_library(serverlib SHARED ${IOC_SERVER_INTERNAL_CODE})
	target_link_libraries(serverlib ${MOTR_LIBRARIES})
	SET_TARGET_PROPERTIES(serverlib PROPERTIES COMPILE_FLAGS "${MOTR_COMPILE_FLAGS}")
else (MERO_FOUND)
	add_library(serverlib SHARED ${IOC_SERVER_INTERNAL_CODE_NOMERO})
endif (MERO_FOUND)
target_link_libraries(serverlib iocatcher-base)

######################################################
if (MERO_FOUND OR MOTR_FOUND)
	add_executable(iocatcher-server ${IOC_SERVER_INTERNAL_CODE} main.cpp)
	target_link_libraries(iocatcher-server iocatcher-base)
	install(TARGETS iocatcher-server DESTINATION ${CMAKE_INSTALL_BINDIR})
	if (MERO_FOUND)
		target_link_libraries(iocatcher-server ${MERO_LIBRARIES})
		SET_TARGET_PROPERTIES(iocatcher-server PROPERTIES COMPILE_FLAGS "${MERO_COMPILE_FLAGS}")
	endif (MERO_FOUND)
	if (MOTR_FOUND)
		target_link_libraries(iocatcher-server ${MOTR_LIBRARIES})
		SET_TARGET_PROPERTIES(iocatcher-server PROPERTIES COMPILE_FLAGS "${MOTR_COMPILE_FLAGS}")
	endif (MOTR_FOUND)
endif (MERO_FOUND OR MOTR_FOUND)

######################################################

######################################################
add_executable(iocatcher-server-no-mero ${IOC_SERVER_INTERNAL_CODE_NOMERO} main.cpp)
target_compile_definitions(iocatcher-server-no-mero PUBLIC -DNOMERO)
target_link_libraries(iocatcher-server-no-mero iocatcher-base)
install(TARGETS iocatcher-server-no-mero DESTINATION ${CMAKE_INSTALL_BINDIR})

######################################################
add_subdirectory(core)
add_subdirectory(hooks)
add_subdirectory(backends)
