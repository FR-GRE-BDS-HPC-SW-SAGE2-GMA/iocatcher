######################################################
#  PROJECT  : IO Catcher                             #
#  LICENSE  : Apache 2.0                             #
#  COPYRIGHT: 2020-2022 Bull SAS All rights reserved #
######################################################

######################################################
set(NETWORK_SRC LibfabricDomain.cpp LibfabricConnection.cpp TcpClient.cpp TcpServer.cpp ClientRegistry.cpp HookLambdaFunction.cpp Protocol.cpp)

######################################################
add_library(ioc-network OBJECT ${NETWORK_SRC})
SET_TARGET_PROPERTIES(ioc-network PROPERTIES POSITION_INDEPENDENT_CODE ON)

######################################################
if (ENABLE_TESTS)
	add_subdirectory(tests)
	add_subdirectory(bench)
endif (ENABLE_TESTS)
