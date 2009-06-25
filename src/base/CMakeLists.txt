# Author: Fred Rothganger
# Copyright 2009 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
# Distributed under the GNU Leser General Public License.  See the file
# LICENSE for details.


add_library (flBase
  ../../include/fl/endian.h
  ../../include/fl/binary.h
  ../../include/fl/parms.h
  ../../include/fl/pointer.h
  ../../include/fl/random.h
  ../../include/fl/serialize.h
  ../../include/fl/stats.h
  ../../include/fl/string.h
  ../../include/fl/time.h
  Parameters.cc
  random.cc
)

install (TARGETS flBase
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
