# Author: Fred Rothganger
# Copyright 2009 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
# Distributed under the GNU Leser General Public License.  See the file
# LICENSE for details.


add_library (flX
  ../../include/fl/X.h
  ../../include/fl/glx.h
  ../../include/fl/gl.h
  ../../include/fl/slideshow.h
  X.cc
  glx.cc
  GLShow.cc
  SlideShow.cc
)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  install (TARGETS flX
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib64
    ARCHIVE DESTINATION lib64
  )
else (CMAKE_SIZEOF_VOID_P EQUAL 8)
  install (TARGETS flX
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )
endif (CMAKE_SIZEOF_VOID_P EQUAL 8)

# No regression tests defined at present.
