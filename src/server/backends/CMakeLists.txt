######################################################
#            PROJECT  : IO Catcher                   #
#            AUTHOR   : Valat Sébastien  - ATOS      #
#            LICENSE  : Apache 2.0                   #
#            COPYRIGHT: 2020 Bull SAS                #
######################################################

######################################################
set(SERVER_BACKEND_SRC StorageBackendMero.cpp
                       MemoryBackendMalloc.cpp
                       MemoryBackendNvdimm.cpp
                       MemoryBackendNvdimmGrow.cpp
                       MemoryBackendCache.cpp
                       MemoryBackendBalance.cpp
)

######################################################

if (MERO_FOUND)
	add_library(ioc-server-backends OBJECT ${SERVER_BACKEND_SRC})
	SET_TARGET_PROPERTIES(ioc-server-backends PROPERTIES POSITION_INDEPENDENT_CODE ON)
	SET_TARGET_PROPERTIES(ioc-server-backends PROPERTIES COMPILE_FLAGS "${MERO_COMPILE_FLAGS}")
endif (MERO_FOUND)
if (MOTR_FOUND)
	add_library(ioc-server-backends OBJECT ${SERVER_BACKEND_SRC})
	SET_TARGET_PROPERTIES(ioc-server-backends PROPERTIES POSITION_INDEPENDENT_CODE ON)
	SET_TARGET_PROPERTIES(ioc-server-backends PROPERTIES COMPILE_FLAGS "${MOTR_COMPILE_FLAGS}")
endif(MOTR_FOUND)

######################################################
add_library(ioc-server-backends-no-mero OBJECT ${SERVER_BACKEND_SRC})
SET_TARGET_PROPERTIES(ioc-server-backends-no-mero PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(ioc-server-backends-no-mero PUBLIC -DNOMERO)

######################################################
if (ENABLE_TESTS)
	add_subdirectory(tests)
endif (ENABLE_TESTS)
