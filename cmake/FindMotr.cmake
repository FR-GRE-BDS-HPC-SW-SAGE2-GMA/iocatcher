# Try to find MOTR libraries
#
# Once done this will define
#
#	MOTR_FOUND - system has MOTR installed

if (MOTR_LIBRARIES)
	SET(MOTR_FOUND TRUE)
else (MOTR_LIBRARIES)
	find_library(MOTR_LIBRARIES NAMES motr)
	find_path(MOTR_INCLUDE_DIRS NAMES motr/client.h PATH_SUFFIXES motr)

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(MOTR DEFAULT_MSG MOTR_LIBRARIES MOTR_INCLUDE_DIRS)
endif (MOTR_LIBRARIES)
