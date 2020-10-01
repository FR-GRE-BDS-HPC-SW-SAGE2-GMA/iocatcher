######################################################
#            PROJECT  : lhcb-daqpipe                 #
#            VERSION  : 2.5.0-dev                    #
#            AUTHOR   : Valat Sébastien              #
#            LICENSE  : Apache 2.0                   #
#            COPYRIGHT: 2020 Bull SAS                #
######################################################

#part of this file is imported from project MALT developped by Sebastien Valat.
#at exascale lab / university of versailles into the MAQAO

######################################################
MACRO(enable_cxx_11)
	include(CheckCXXCompilerFlag)
	CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
	if(COMPILER_SUPPORTS_CXX11)
		message(STATUS "Using -std=c++11")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	else()
			message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
	endif()
ENDMACRO(enable_cxx_11)
