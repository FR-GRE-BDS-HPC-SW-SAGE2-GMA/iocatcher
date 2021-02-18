######################################################
#            PROJECT  : IO Catcher                   #
#            AUTHOR   : Valat Sébastien  - ATOS      #
#            LICENSE  : Apache 2.0                   #
#            COPYRIGHT: 2020 Bull SAS                #
######################################################

######################################################
set(IOC_CLIENT_INTERNAL_CODE
	# extern deps
	$<TARGET_OBJECTS:daqpipe-common>
	# internal deps
	$<TARGET_OBJECTS:ioc-network>
	# source code
	Actions.cpp
	ioc-client.cpp
)

######################################################
add_executable(iocatcher-client main.cpp ${IOC_CLIENT_INTERNAL_CODE})
install(TARGETS iocatcher-client DESTINATION ${CMAKE_INSTALL_BINDIR})
target_link_libraries(iocatcher-client pthread)
target_link_libraries(iocatcher-client ${LIBFABRIC_LIBRARIES})
target_link_libraries(iocatcher-client ${LIBEVENT_LIBRARIES} ${LIBEVENT_PTHREADS_LIBRARIES})

######################################################
add_library(ioc-client SHARED ${IOC_CLIENT_INTERNAL_CODE})
install(TARGETS ioc-client DESTINATION lib)
install(FILES ioc-client.h DESTINATION include)
target_link_libraries(ioc-client pthread)
target_link_libraries(ioc-client ${LIBFABRIC_LIBRARIES})
target_link_libraries(ioc-client ${LIBEVENT_LIBRARIES} ${LIBEVENT_PTHREADS_LIBRARIES})
