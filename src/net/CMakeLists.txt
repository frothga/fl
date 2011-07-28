# Author: Fred Rothganger
# Copyright 2009, 2010 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
# Distributed under the GNU Leser General Public License.  See the file
# LICENSE for details.


add_library (flNet
  ../../include/fl/socket.h
  Listener.cc
  SocketStream.cc
)
target_link_libraries (flNet
  ${FL_LIBRARIES}
)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  install (TARGETS flNet
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib64
    ARCHIVE DESTINATION lib64
  )
else (CMAKE_SIZEOF_VOID_P EQUAL 8)
  install (TARGETS flNet
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )
endif (CMAKE_SIZEOF_VOID_P EQUAL 8)

add_executable (testNet test.cc)
target_link_libraries (testNet
  flNet
  ${FL_LIBRARIES}
)
