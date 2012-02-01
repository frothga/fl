# Author: Fred Rothganger
# Copyright 2009 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
# Distributed under the GNU Leser General Public License.  See the file
# LICENSE for details.


add_library (flBase
  ../../include/fl/archive.h
  ../../include/fl/binary.h
  ../../include/fl/endian.h
  ../../include/fl/metadata.h
  ../../include/fl/parms.h
  ../../include/fl/pointer.h
  ../../include/fl/string.h
  ../../include/fl/time.h
  Archive.cc
  Metadata.cc
  NamedValueSet.cc
  Parameters.cc
  Pointer.cc
)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  install (TARGETS flBase
    RUNTIME DESTINATION bin64
    LIBRARY DESTINATION lib64
    ARCHIVE DESTINATION lib64
  )
else (CMAKE_SIZEOF_VOID_P EQUAL 8)
  install (TARGETS flBase
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )
endif (CMAKE_SIZEOF_VOID_P EQUAL 8)

add_executable (testBase test.cc)
target_link_libraries (testBase
  flBase
)
