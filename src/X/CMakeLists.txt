# Author: Fred Rothganger
# Copyright 2009, 2010 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
# Distributed under the GNU Leser General Public License.  See the file
# LICENSE for details.


if (MSVC)
  add_library (flX
    ../../include/fl/slideshow.h
    SlideShowForWindows.cc
  )
else (MSVC)
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
endif (MSVC)

target_link_libraries (flX
  flImage
  flNumeric
  flBase
  ${FL_LIBRARIES}
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
