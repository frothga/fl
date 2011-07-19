# - Find pthread library
# This module defines
#   Pthread_FOUND         If false, do not try to use Pthread
#   Pthread_LIBRARIES     The library to link against


find_library (Pthread_LIB
  NAMES pthread pthreads pthreadVC2
)

include (FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS (Pthread  DEFAULT_MSG  Pthread_LIB)

if (Pthread_FOUND)
  set (Pthread_LIBRARIES ${Pthread_LIB})
endif (Pthread_FOUND)

mark_as_advanced (Pthread_LIB)
