######################################################
#  PROJECT  : IO Catcher                             #
#  LICENSE  : Apache 2.0                             #
#  COPYRIGHT: 2020-2022 Bull SAS All rights reserved #
######################################################

######################################################
set(IOC_BASE_INTERNAL_CODE
	#extern deps
	$<TARGET_OBJECTS:daqpipe-common>
	#internal
#	$<TARGET_OBJECTS:ioc-common>
	$<TARGET_OBJECTS:ioc-network>
	#we should keep an eye on portability of this
#	$<TARGET_OBJECTS:ioc-portability>
	#optional
#	${OPTIONAL_EXTRA}
)

######################################################
set(IOC_BASE_EXTERN_LIBS ${LIBFABRIC_LIBRARIES})

######################################################
add_library(iocatcher-base STATIC ${IOC_BASE_INTERNAL_CODE})
set_target_properties(iocatcher-base PROPERTIES VERSION 0.0.0 SOVERSION 0)
target_link_libraries(iocatcher-base ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(iocatcher-base ${IOC_BASE_EXTERN_LIBS})
target_link_libraries(iocatcher-base ${LIBEVENT_LIBRARIES} ${LIBEVENT_PTHREADS_LIBRARIES})

######################################################
add_subdirectory(common)
add_subdirectory(network)
