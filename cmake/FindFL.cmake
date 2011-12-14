# - Find FL and its supporting libraries
# This module defines
#   FL_FOUND         If false, do not try to use FL.
#   FL_INCLUDE_DIRS  Directories containing the include files
#   FL_LIBRARIES     Full path to libraries to link against

# Author: Fred Rothganger
# Copyright 2010 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
# Distributed under the GNU Leser General Public License.  See the file
# LICENSE for details.


set (FL_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/.. CACHE PATH "Base directory containing FL installation.  This would be the directory that contains bin, lib, include, etc., for FL.")

if    (CMAKE_CL_64)
  set_property (GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS TRUE)
endif (CMAKE_CL_64)

set (CMAKE_PREFIX_PATH ${FL_ROOT_DIR})

find_path (FL_INCLUDE_DIRS fl/image.h)

find_library (FL_X       flX)
find_library (FL_IMAGE   flImage)
find_library (FL_NUMERIC flNumeric)
find_library (FL_BASE    flBase)
find_library (FL_NET     flNet)

set (FL_LIBRARIES)
if (NOT FL_BUILD)
  if    (FL_X)
    set (FL_LIBRARIES ${FL_LIBRARIES} ${FL_X})
  endif (FL_X)

  if    (FL_IMAGE)
    set (FL_LIBRARIES ${FL_LIBRARIES} ${FL_IMAGE})
  endif (FL_IMAGE)

  if    (FL_NUMERIC)
    set (FL_LIBRARIES ${FL_LIBRARIES} ${FL_NUMERIC})
  endif (FL_NUMERIC)

  if    (FL_BASE)
    set (FL_LIBRARIES ${FL_LIBRARIES} ${FL_BASE})
  endif (FL_BASE)

  if    (FL_NET)
    set (FL_LIBRARIES ${FL_LIBRARIES} ${FL_NET})
  endif (FL_NET)
endif (NOT FL_BUILD)


# handle the QUIETLY and REQUIRED arguments
include (FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FL  DEFAULT_MSG  FL_LIBRARIES FL_INCLUDE_DIRS)


# Prioritize /usr ahead of / when searching for libraries.  Work-around for
# interaction between encap symbolic links and directory shadowing under Cygwin.
set (CMAKE_PREFIX_PATH /usr $ENV{MKL_HOME})

set (CMAKE_MODULE_PATH ${FL_ROOT_DIR}/cmake)
if (MSVC)
  set (CMAKE_INCLUDE_PATH ${FL_ROOT_DIR}/mswin/include)
  if    (CMAKE_CL_64)
    set (CMAKE_LIBRARY_PATH ${FL_ROOT_DIR}/mswin/lib64)
  else  (CMAKE_CL_64)
    set (CMAKE_LIBRARY_PATH ${FL_ROOT_DIR}/mswin/lib)
  endif (CMAKE_CL_64)

  # Add compiler support libraries.
  find_library (FORTRAN_LIB  fortran)
  if    (FORTRAN_LIB)
    set (GCC_LIBRARIES ${GCC_LIBRARIES} ${FORTRAN_LIB})
  endif (FORTRAN_LIB)

  set (SOCKET_LIBRARIES ws2_32)

  # Some includes are available by default under Unix but are not available
  # under MSVC, so add the supplemental include directory preemtively.
  set (FL_INCLUDE_DIRS ${FL_INCLUDE_DIRS} ${FL_ROOT_DIR}/mswin/include)

  # Avoid linkage conflicts caused by using mswin libraries.
  if    (NOT CMAKE_EXE_LINKER_FLAGS MATCHES libcmt)
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:libcmt;libcmtd /OPT:NOREF" CACHE STRING "linker flags" FORCE)
  endif (NOT CMAKE_EXE_LINKER_FLAGS MATCHES libcmt)

  if    (NOT CMAKE_SHARED_LINKER_FLAGS MATCHES libcmt)
    set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /NODEFAULTLIB:libcmt;libcmtd /OPT:NOREF" CACHE STRING "linker flags" FORCE)
  endif (NOT CMAKE_SHARED_LINKER_FLAGS MATCHES libcmt)

  # Use multiple-processors to compile c/c++ code
  if    (NOT CMAKE_CXX_FLAGS MATCHES /MP)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP" CACHE STRING "C++ compiler flags" FORCE)
    set (CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   /MP" CACHE STRING "C compiler flags"   FORCE)
  endif (NOT CMAKE_CXX_FLAGS MATCHES /MP)

else (MSVC)

  # VC Express chokes on CMake's Fortran test, so only do it under a non-VC platform.
  enable_language (Fortran OPTIONAL)

  if     (CMAKE_Fortran_COMPILER MATCHES gfortran|f95)
    set (FORTRAN_LIB gfortran)
  elseif (CMAKE_Fortran_COMPILER MATCHES g77)
    set (FORTRAN_LIB g2c)
  else   (CMAKE_Fortran_COMPILER MATCHES gfortran|f95)
    set (FORTRAN_LIB "")
  endif  (CMAKE_Fortran_COMPILER MATCHES gfortran|f95)

  set (GCC_LIBRARIES ${FORTRAN_LIB})

endif (MSVC)

# Under Linux, we want to link with librt in order to use finer grained timers.
if    (CMAKE_SYSTEM_NAME MATCHES Linux)
  find_library (RT_LIBRARIES rt)
else  (CMAKE_SYSTEM_NAME MATCHES Linux)
  set          (RT_LIBRARIES)
endif (CMAKE_SYSTEM_NAME MATCHES Linux)

# Preempt FindThreads to ensure that pthreads will be used under Windows
find_library (CMAKE_THREAD_LIBS_INIT pthread)
find_package (Threads)

get_property (LANGUAGES GLOBAL PROPERTY ENABLED_LANGUAGES)
if    (LANGUAGES MATCHES Fortran)
  # Standard package finders, which depend on availability of Fortran...
  find_package (LAPACK)
else  (LANGUAGES MATCHES Fortran)
  # ...but Fortran really isn't necessary for finding these packages.
  find_package (LAPACKnofortran)
endif (LANGUAGES MATCHES Fortran)

find_package (FFTW)
find_package (TIFF)
find_package (GTIF)
find_package (JPEG)
find_package (PNG)
find_package (Freetype)
find_package (X11)
find_package (OpenGL)
find_package (FFMPEG)

set (FL_LIBRARIES ${FL_LIBRARIES}
  ${FFTW_LIBRARIES}
  ${TIFF_LIBRARIES}
  ${GTIF_LIBRARIES}
  ${JPEG_LIBRARIES}
  ${PNG_LIBRARIES}
  ${FREETYPE_LIBRARIES}
  ${X11_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${FFMPEG_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${GCC_LIBRARIES}
  ${RT_LIBRARIES}
  ${SOCKET_LIBRARIES}
)

if    (CMAKE_USE_PTHREADS_INIT)
  add_definitions (-DHAVE_PTHREAD)
endif (CMAKE_USE_PTHREADS_INIT)

if    (LAPACK_FOUND)
  add_definitions (-DHAVE_LAPACK)
  # The standard FindLAPACK and FindBLAS leave a "FALSE" in LIBRARIES
  # so they can't be used directly above.
  set (FL_LIBRARIES ${FL_LIBRARIES} ${LAPACK_LIBRARIES})
endif (LAPACK_FOUND)

if    (BLAS_FOUND)
  add_definitions (-DHAVE_BLAS)
  if    (NOT LAPACK_FOUND)
    set (FL_LIBRARIES ${FL_LIBRARIES} ${BLAS_LIBRARIES})
  endif (NOT LAPACK_FOUND)
endif (BLAS_FOUND)

if    (FFTW_FOUND)
  add_definitions (-DHAVE_FFTW)
  # For the most part, include paths are standard and don't need to be
  # added.  However, on HPC systems FFTW gets special treatment so needs
  # special pathing.
  set (FL_INCLUDE_DIRS ${FL_INCLUDE_DIRS} ${FFTW_INCLUDE_DIRS})
endif (FFTW_FOUND)

if    (TIFF_FOUND)
  add_definitions (-DHAVE_TIFF)
endif (TIFF_FOUND)

if    (GTIF_FOUND)
  add_definitions (-DHAVE_GTIF)
endif (GTIF_FOUND)

if    (JPEG_FOUND)
  add_definitions (-DHAVE_JPEG)
endif (JPEG_FOUND)

if    (PNG_FOUND)
  add_definitions (-DHAVE_PNG)
endif (PNG_FOUND)

if    (FREETYPE_FOUND)
  add_definitions (-DHAVE_FREETYPE)
  set (FL_INCLUDE_DIRS ${FL_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS})
endif (FREETYPE_FOUND)

if    (X11_FOUND)
  add_definitions (-DHAVE_X11)
endif (X11_FOUND)

if    (FFMPEG_FOUND)
  add_definitions (-DHAVE_FFMPEG)
endif (FFMPEG_FOUND)

mark_as_advanced (
  FL_INCLUDE_DIRS
  FL_LIBRARIES
  FL_X
  FL_IMAGE
  FL_NUMERIC
  FL_BASE
  FL_NET
  RT_LIBRARIES
  CMAKE_THREAD_LIBS_INIT
)
