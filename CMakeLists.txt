# Author: Fred Rothganger
# Copyright 2009, 2010 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
# Distributed under the GNU Leser General Public License.  See the file
# LICENSE for details.


project (FL)
cmake_minimum_required (VERSION 2.6)
option (BUILD_SHARED_LIBS "Generate shared rather than static libraries" OFF)


set (CMAKE_MODULE_PATH cmake)
find_package (FL)

include_directories (${FL_INCLUDE_DIRS})


# Set destination for targets -------------------------------------------------

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  if (MSVC)
    # Install to the root of the source directory.  This compensates for
    # the VS generator's (apparent) lack of ability to cleanly place just
    # the target file in the desired output directory.  Try to discover a
    # cleaner way to do this so we don't overload the install process.
    set (CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR} CACHE PATH "Where to install" FORCE)
  else (MSVC)
    # An arbitrary install location consistent with the use of the Encap package manager on *nix systems
    set (CMAKE_INSTALL_PREFIX /usr/encap/fl CACHE PATH "Where to install" FORCE)
  endif (MSVC)
endif (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

# See above comment on MSVC install prefix.  The VS generator places some
# intermediate files in the output directory, which is ugly.
if (NOT MSVC)
  set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib64)
    set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib64)
  else (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
    set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
  endif (CMAKE_SIZEOF_VOID_P EQUAL 8)
  install (DIRECTORY script/
    DESTINATION bin
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  )
endif (NOT MSVC)

install (DIRECTORY include/fl DESTINATION include)


# Add all targets -------------------------------------------------------------

add_subdirectory (src/base)
add_subdirectory (src/net)
add_subdirectory (src/numeric)
add_subdirectory (src/image)
if (X11_FOUND  OR  MSVC)
  add_subdirectory (src/X)
endif (X11_FOUND  OR  MSVC)
if (NOT MSVC)
  add_subdirectory (src/util)
endif (NOT MSVC)
