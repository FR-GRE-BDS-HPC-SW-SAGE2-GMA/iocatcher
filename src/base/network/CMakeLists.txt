######################################################
#            PROJECT  : IO Catcher                   #
#            AUTHOR   : Valat Sébastien  - ATOS      #
#            LICENSE  : Apache 2.0                   #
#            COPYRIGHT: 2020 Bull SAS                #
######################################################

######################################################
set(NETWORK_SRC LibfabricDomain.cpp LibfabricConnection.cpp TcpClient.cpp TcpServer.cpp ClientRegistry.cpp)

######################################################
add_library(ioc-network OBJECT ${NETWORK_SRC})
SET_TARGET_PROPERTIES(ioc-network PROPERTIES POSITION_INDEPENDENT_CODE ON)

######################################################
if (ENABLE_TESTS)
	add_subdirectory(tests)
endif (ENABLE_TESTS)
