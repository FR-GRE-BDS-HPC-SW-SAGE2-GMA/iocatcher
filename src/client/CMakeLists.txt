######################################################
#            PROJECT  : IO Catcher                   #
#            AUTHOR   : Valat Sébastien  - ATOS      #
#            LICENSE  : Apache 2.0                   #
#            COPYRIGHT: 2020 Bull SAS                #
######################################################

######################################################
set(IOC_CLIENT_INTERNAL_CODE
	#extern deps
#	$<TARGET_OBJECTS:daqpipe-common>
	#internal
#	$<TARGET_OBJECTS:ioc-common>
#	$<TARGET_OBJECTS:ioc-network>
	#we should keep an eye on portability of this
#	$<TARGET_OBJECTS:ioc-portability>
	#optional
#	${OPTIONAL_EXTRA}
	main.cpp
)

######################################################
add_executable(iocatcher-client main.cpp Actions.cpp ioc-client.cpp)
target_link_libraries(iocatcher-client iocatcher-base)
install(TARGETS iocatcher-client DESTINATION ${CMAKE_INSTALL_BINDIR})

######################################################
add_library(ioc-client SHARED Actions.cpp ioc-client.cpp)
target_link_libraries(ioc-client iocatcher-base)
install(TARGETS ioc-client DESTINATION lib)
install(FILES ioc-client.h DESTINATION include)
