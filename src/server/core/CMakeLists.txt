######################################################
#            PROJECT  : IO Catcher                   #
#            AUTHOR   : Valat Sébastien  - ATOS      #
#            LICENSE  : Apache 2.0                   #
#            COPYRIGHT: 2020 Bull SAS                #
######################################################

######################################################
set(SERVER_CORE_SRC Object.cpp Container.cpp)

######################################################
add_library(ioc-server-core OBJECT ${SERVER_CORE_SRC} clovis_api.c)
SET_TARGET_PROPERTIES(ioc-server-core PROPERTIES POSITION_INDEPENDENT_CODE ON)
SET_TARGET_PROPERTIES(ioc-server-core PROPERTIES COMPILE_FLAGS "${MERO_COMPILE_FLAGS}")

######################################################
add_library(ioc-server-core-no-mero OBJECT ${SERVER_CORE_SRC})
SET_TARGET_PROPERTIES(ioc-server-core-no-mero PROPERTIES POSITION_INDEPENDENT_CODE ON)
SET_TARGET_PROPERTIES(ioc-server-core-no-mero PROPERTIES COMPILE_FLAGS "${MERO_COMPILE_FLAGS}")
target_compile_definitions(ioc-server-core-no-mero PUBLIC -DNOMERO)

######################################################
if (ENABLE_TESTS)
	add_subdirectory(tests)
endif (ENABLE_TESTS)
