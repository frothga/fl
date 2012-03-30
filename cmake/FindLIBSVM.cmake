# - Find LIBSVM library
# Find the native LIBSVM includes and library
# This module defines
#   LIBSVM_FOUND         If false, do not try to use LIBSVM.
#   LIBSVM_INCLUDE_DIRS  Directory containing the include files
#   LIBSVM_LIBRARIES     Full path to library to link against

find_path (LIBSVM_INCLUDE_DIRS
  NAMES svm.h
  PATH_SUFFIXES libsvm
)

find_library (LIBSVM_LIBRARIES svm)

# handle the QUIETLY and REQUIRED arguments and set LIBSVM_FOUND to TRUE if 
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBSVM  DEFAULT_MSG  LIBSVM_LIBRARIES LIBSVM_INCLUDE_DIRS)

mark_as_advanced (
  LIBSVM_INCLUDE_DIRS
  LIBSVM_LIBRARIES
)
