# Author: Fred Rothganger
# Copyright 2009 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
# Distributed under the GNU Leser General Public License.  See the file
# LICENSE for details.


add_executable (addm addm.cc)

add_executable (convertall convertall.cc)

add_executable (epsall epsall.cc)

add_executable (invert invert.cc)
target_link_libraries (invert
  flImage
  flNumeric
  flBase
  ${LAPACK_LIBRARIES}
  ${GCC_LIBRARIES}
)

add_executable (psnr psnr.cc)
target_link_libraries (psnr
  flImage
  flNumeric
  flBase
  ${FFMPEG_LIBRARIES}
  ${GCC_LIBRARIES}
  ${RT_LIB}
)

add_executable (stripm stripm.cc)

add_executable (striputf8 striputf8.cc)

add_executable (thumb thumb.cc)
target_link_libraries (thumb
  flImage
  flNumeric
  flBase
  ${FFMPEG_LIBRARIES}
  ${JPEG_LIBRARIES}
  ${LAPACK_LIBRARIES}
  ${GCC_LIBRARIES}
  ${RT_LIB}
)

add_executable (vread vread.cc)
target_link_libraries (vread
  flX
  flImage
  flNumeric
  flBase
  ${FFMPEG_LIBRARIES}
  ${JPEG_LIBRARIES}
  ${X11_LIBRARIES}
  ${GCC_LIBRARIES}
  ${RT_LIB}
)

add_executable (xv xv.cc)
target_link_libraries (xv
  flX
  flImage
  flNumeric
  flBase
  ${X11_LIBRARIES}
  ${GTIF_LIBRARIES}
  ${TIFF_LIBRARIES}
  ${JPEG_LIBRARIES}
  ${PNG_LIBRARIES}
  ${LAPACK_LIBRARIES}
  ${GCC_LIBRARIES}
  ${RT_LIB}
)

install (TARGETS addm convertall epsall invert psnr stripm striputf8 thumb vread xv
  RUNTIME DESTINATION bin
)
