######################################################
#            PROJECT  : lhcb-daqpipe                 #
#            VERSION  : 2.5.0-dev                    #
#            DATE     : 12/2017                      #
#            AUTHOR   : Valat Sébastien - CERN       #
#            LICENSE  : CeCILL-C                     #
######################################################

######################################################
#Use gtest
include_directories(${GTEST_INCLUDE_DIRS} ${GMOCK_INCLUDE_DIRS})
include_directories(../)

######################################################
set(TEST_NAMES TestDebug
               TestFormattedMessage)

######################################################
FOREACH(test_name ${TEST_NAMES})
	add_executable(${test_name} ${test_name}.cpp $<TARGET_OBJECTS:daqpipe-common>)
	target_link_libraries(${test_name} ${GTEST_BOTH_LIBRARIES} ${GMOCK_BOTH_LIBRARIES})
	ioc_add_test(${test_name} ${test_name})
ENDFOREACH(test_name)
