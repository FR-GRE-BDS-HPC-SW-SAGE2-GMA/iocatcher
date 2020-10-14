######################################################
#            PROJECT  : cdeps                        #
#            VERSION  : 0.0.0-dev                    #
#            DATE     : 10/2018                      #
#            LICENSE  : CeCILL-C                     #
######################################################

######################################################
include_directories(${CMAKE_BINARY_DIR}/src)

######################################################
add_library(daqpipe-common OBJECT FormattedMessage.cpp Debug.cpp)
SET_TARGET_PROPERTIES(daqpipe-common PROPERTIES COMPILE_FLAGS -fPIC)

######################################################
if (ENABLE_TESTS)
	add_subdirectory(tests)
endif (ENABLE_TESTS)