# Author: Fred Rothganger
# Copyright 2009 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
# Distributed under the GNU Leser General Public License.  See the file
# LICENSE for details.


add_library (flNet
  ../../include/fl/socket.h
  Listener.cc
  SocketStream.cc
)

install (TARGETS flNet
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

add_executable (testNet
  test.cc
)

target_link_libraries (testNet
  flNet
  ${THREAD_LIB}
  ${SOCKET_LIB}
)
