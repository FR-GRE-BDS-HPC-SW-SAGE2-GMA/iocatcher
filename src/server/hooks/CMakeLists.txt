######################################################
#            PROJECT  : IO Catcher                   #
#            LICENSE  : Apache 2.0                   #
#            COPYRIGHT: 2020 Bull SAS                #
######################################################

######################################################
set(SERVER_HOOKS_SRC HookPingPong.cpp 
                     HookFlush.cpp
                     HookRangeRegister.cpp
                     HookRangeUnregister.cpp
                     HookObjectCreate.cpp
                     HookObjectRead.cpp
                     HookObjectWrite.cpp
                     HookObjectCow.cpp
)

######################################################
add_library(ioc-server-hooks OBJECT ${SERVER_HOOKS_SRC})
SET_TARGET_PROPERTIES(ioc-server-hooks PROPERTIES POSITION_INDEPENDENT_CODE ON)

######################################################
if (ENABLE_TESTS)
	add_subdirectory(tests)
endif (ENABLE_TESTS)
