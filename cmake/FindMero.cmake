# Try to find MERO libraries
#
# Once done this will define
#
#	MERO_FOUND - system has MERO installed

if (MERO_LIBRARIES)
	SET(MERO_FOUND TRUE)
else (MERO_LIBRARIES)

	find_library(MERO_LIBRARIES NAMES mero)
	find_path(MERO_INCLUDE_DIRS NAMES clovis/clovis.h PATH_SUFFIXES mero)

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(MERO DEFAULT_MSG MERO_LIBRARIES MERO_INCLUDE_DIRS)

endif (MERO_LIBRARIES)
