# - Find FFMPEG library
# Find the native FFMPEG includes and library
# This module defines
#   FFMPEG_FOUND        If false, do not try to use FFMPEG.
#   FFMPEG_INCLUDE_DIRS Directories containing the include files
#   FFMPEG_AVCODEC      Full path to libavcodec, and similarly for each of the
#   FFMPEG_AVDEVICE     other libraries.
#   FFMPEG_AVFORMAT
#   FFMPEG_AVUTIL
#   FFMPEG_SWSCALE
#   FFMPEG_LIBRARIES    All the available libraries together.
# FFMPEG is always evolving.  This script makes no attempt to be general
# across multiple generations of FFMPEG.  Instead, it must be maintained to
# find the most current directory layout.  An alternative would be to detect
# several different layouts and lable them with version numbers.  However,
# the FFMPEG project does not have a very strong formal release process.


# Whether zlib is necessary or not depends on the configuration of FFMPEG.
find_package (ZLIB)
find_package (BZip2)

find_path (FFMPEG_INCLUDE_DIRS libavformat/avformat.h
  PATH_SUFFIXES
    ffmpeg
)

find_library (FFMPEG_AVFORMAT avformat)
find_library (FFMPEG_AVCODEC  avcodec)
find_library (FFMPEG_AVUTIL   avutil)
find_library (FFMPEG_AVDEVICE avdevice)
find_library (FFMPEG_SWSCALE  swscale)

# handle the QUIETLY and REQUIRED arguments and set FFMPEG_FOUND to TRUE if 
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FFMPEG  DEFAULT_MSG  FFMPEG_INCLUDE_DIRS FFMPEG_AVFORMAT FFMPEG_AVCODEC FFMPEG_AVUTIL)

if (FFMPEG_FOUND)
  set (FFMPEG_LIBRARIES ${FFMPEG_AVFORMAT} ${FFMPEG_AVCODEC} ${FFMPEG_AVUTIL})
else (FFMPEG_FOUND)
  set (FFMPEG_LIBRARIES)
endif (FFMPEG_FOUND)
if (FFMPEG_AVDEVICE)
  set (FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} ${FFMPEG_AVDEVICE})
endif (FFMPEG_AVDEVICE)
if (FFMPEG_SWSCALE)
  set (FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} ${FFMPEG_SWSCALE})
endif (FFMPEG_SWSCALE)

if (ZLIB_FOUND)
  set (FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} ${ZLIB_LIBRARIES})
endif (ZLIB_FOUND)
if (BZIP2_FOUND)
  set (FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} ${BZIP2_LIBRARIES})
endif (BZIP2_FOUND)
if (THREAD_LIB)
  set (FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} ${THREAD_LIB})
endif (THREAD_LIB)

#message ("ffmpeg_libraries = ${FFMPEG_LIBRARIES}")

mark_as_advanced (
  FFMPEG_INCLUDE_DIR
  FFMPEG_LIBRARIES
  FFMPEG_AVFORMAT
  FFMPEG_AVCODEC
  FFMPEG_AVUTIL
  FFMPEG_AVDEVICE
  FFMPEG_SWSCALE
)
