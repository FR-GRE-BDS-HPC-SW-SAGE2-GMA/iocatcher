######################################################
#  PROJECT  : IO Catcher                             #
#  LICENSE  : Apache 2.0                             #
#  COPYRIGHT: 2020-2022 Bull SAS All rights reserved #
######################################################

######################################################
set(SERVER_CORE_SRC Object.cpp 
                    ObjectSegment.cpp
                    Container.cpp
                    ConsistencyTracker.cpp
                    Server.cpp Config.cpp
                    StorageBackend.cpp
                    MemoryBackend.cpp
)

######################################################
add_library(ioc-server-core OBJECT ${SERVER_CORE_SRC})
SET_TARGET_PROPERTIES(ioc-server-core PROPERTIES POSITION_INDEPENDENT_CODE ON)

######################################################
if (ENABLE_TESTS)
	add_subdirectory(tests)
endif (ENABLE_TESTS)
