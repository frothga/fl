# - Find BLAS library
# This module follows (more or less) the same convention as FindBLAS, but
# does not depend on using a Fortran compiler to probe the library.
# It defines
#   BLAS_FOUND         If false, do not try to use BLAS
#   BLAS_LIBRARIES     The library to link against
#   BLAS_LINKER_FLAGS  uncached list of required linker flags (excluding -l
#                      and -L)

find_library (BLAS_LIBRARIES blas)

# handle the QUIETLY and REQUIRED arguments and set BLAS_FOUND to TRUE if 
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS (BLAS  DEFAULT_MSG  BLAS_LIBRARIES)

set (BLAS_LINKER_FLAGS)

mark_as_advanced (
  BLAS_LIBRARIES
  BLAS_LINKER_FLAGS
)
