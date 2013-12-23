# - Find GTIF library
# Find the native GTIF includes and library
# This module defines
#   GTIF_FOUND  If false, do not try to use GTIF.
#   GTIF_INC    Directory containing the include files
#   GTIF_LIB    Full path to library to link against

find_path (GTIF_INC geotiff.h
  PATH_SUFFIXES
    libgeotiff
)

find_library (GTIF_LIB geotiff)

# handle the QUIETLY and REQUIRED arguments and set GTIF_FOUND to TRUE if 
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GTIF  DEFAULT_MSG  GTIF_LIB GTIF_INC)

mark_as_advanced (
  GTIF_INC
  GTIF_LIB
)
