######################################################
#            PROJECT  : IO Catcher                   #
#            LICENSE  : Apache 2.0                   #
#            COPYRIGHT: 2020 Bull SAS                #
######################################################

#part of this file is imported from project MALT developped by Sebastien Valat.
#at exascale lab / university of versailles into the MAQAO

######################################################
#Setup paths to gtest/gmock headers and library
MACRO(setup_internal_gmock_and_gtest)
	set(GMOCK_SOURCE_DIR ${CMAKE_SOURCE_DIR}/extern-deps/googletest-release-1.10.0)
	set(GMOCK_INCLUDE_DIR ${GMOCK_SOURCE_DIR}/include)
	set(GMOCK_INCLUDE_DIRS ${GMOCK_SOURCE_DIR}/include)
	set(GMOCK_BOTH_LIBRARIES gmock gmock_main)
	set(GTEST_BOTH_LIBRARIES gtest)
	set(GTEST_INCLUDE_DIR ${GMOCK_SOURCE_DIR}/gtest/include/)
	set(GTEST_INCLUDE_DIRS ${GMOCK_SOURCE_DIR}/gtest/include/)
ENDMACRO(setup_internal_gmock_and_gtest)

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

######################################################
MACRO(enable_gcc_coverage)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -fprofile-arcs -ftest-coverage")
        set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -O0 -fprofile-arcs -ftest-coverage")
        set(CMAKE_EXE_LINKER_FLAGS_FLAGS "${CMAKE_C_FLAGS} -O0 -fprofile-arcs -ftest-coverage")
ENDMACRO(enable_gcc_coverage)
