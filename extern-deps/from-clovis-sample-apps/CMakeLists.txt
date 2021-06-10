######################################################
#            PROJECT  : IO Catcher                   #
#            AUTHOR   : Valat Sébastien  - ATOS      #
#            LICENSE  : Apache 2.0                   #
#            COPYRIGHT: 2020 Bull SAS                #
######################################################

######################################################
set(MERO_SRC clovis_api.c)

######################################################
if (MERO_FOUND)
	add_library(ioc-extern-mero OBJECT ${MERO_SRC})
	SET_TARGET_PROPERTIES(ioc-extern-mero PROPERTIES POSITION_INDEPENDENT_CODE ON)
	SET_TARGET_PROPERTIES(ioc-extern-mero PROPERTIES COMPILE_FLAGS "${MERO_COMPILE_FLAGS}")
endif(MERO_FOUND)
