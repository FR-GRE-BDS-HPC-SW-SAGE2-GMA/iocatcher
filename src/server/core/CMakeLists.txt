######################################################
#            PROJECT  : IO Catcher                   #
#            AUTHOR   : Valat Sébastien  - ATOS      #
#            LICENSE  : Apache 2.0                   #
#            COPYRIGHT: 2020 Bull SAS                #
######################################################

######################################################
set(SERVER_CORE_SRC Object.cpp Container.cpp ConsistencyTracker.cpp Server.cpp Config.cpp StorageBackend.cpp)

######################################################
add_library(ioc-server-core OBJECT ${SERVER_CORE_SRC})
SET_TARGET_PROPERTIES(ioc-server-core PROPERTIES POSITION_INDEPENDENT_CODE ON)

######################################################
if (ENABLE_TESTS)
	add_subdirectory(tests)
endif (ENABLE_TESTS)
