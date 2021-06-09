######################################################
#            PROJECT  : IO Catcher                   #
#            AUTHOR   : Valat Sébastien  - ATOS      #
#            LICENSE  : Apache 2.0                   #
#            COPYRIGHT: 2020 Bull SAS                #
######################################################

######################################################
set(MOTR_SRC clovis_api.c
             buffer.c
             perf.c
             qos.c)

######################################################
if (MOTR_FOUND)
	add_library(ioc-extern-motr OBJECT ${MOTR_SRC})
	SET_TARGET_PROPERTIES(ioc-extern-motr PROPERTIES POSITION_INDEPENDENT_CODE ON)
	SET_TARGET_PROPERTIES(ioc-extern-motr PROPERTIES COMPILE_FLAGS "${MOTR_COMPILE_FLAGS}")
endif(MOTR_FOUND)
