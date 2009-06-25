# - Find LAPACK library
# This module follows (more or less) the same convention as FindLAPACK, but
# does not depend on using a Fortran compiler to probe the library.
# It defines
#   LAPACK_FOUND         If false, do not try to use LAPACK
#   LAPACK_LIBRARIES     The library to link against
#   LAPACK_LINKER_FLAGS  uncached list of required linker flags (excluding -l
#                        and -L)

find_package (BLASnofortran)

find_library (LAPACK_LIB lapack)

# handle the QUIETLY and REQUIRED arguments and set TIFF_FOUND to TRUE if 
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS (LAPACK  DEFAULT_MSG  LAPACK_LIB BLAS_FOUND)

if (LAPACK_FOUND)
  set (LAPACK_LIBRARIES ${LAPACK_LIB} ${BLAS_LIBRARIES})
endif (LAPACK_FOUND)

set (LAPACK_LINKER_FLAGS)

mark_as_advanced (
  LAPACK_LIB
  LAPACK_LIBRARIES
  LAPACK_LINKER_FLAGS
)
