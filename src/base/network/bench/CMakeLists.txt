######################################################
#            PROJECT  : IO Catcher                   #
#            AUTHOR   : Valat Sébastien  - ATOS      #
#            LICENSE  : Apache 2.0                   #
#            COPYRIGHT: 2020 Bull SAS                #
######################################################

######################################################
#Use gtest
include_directories(${GTEST_INCLUDE_DIRS} ${GMOCK_INCLUDE_DIRS})
include_directories(../)

######################################################
set(BENCH_NAMES BenchLibfabricDomain)

######################################################
FOREACH(test_name ${BENCH_NAMES})
	add_executable(${test_name} ${test_name}.cpp)
	target_link_libraries(${test_name} serverlib)
ENDFOREACH(test_name)
