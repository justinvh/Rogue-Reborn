#-------------------------------------------------------------------------------
#                ______             ______ ____          __  __
#               |  ____|           |  ____/ __ \   /\   |  \/  |
#               | |__ _ __ ___  ___| |__ | |  | | /  \  | \  / |
#               |  __| '__/ _ \/ _ \  __|| |  | |/ /\ \ | |\/| |
#               | |  | | |  __/  __/ |   | |__| / ____ \| |  | |
#               |_|  |_|  \___|\___|_|    \____/_/    \_\_|  |_|
#
#                   FreeFOAM: The Cross-Platform CFD Toolkit
#
# Copyright (C) 2009 Michael Wild <themiwi@users.sf.net>
#                    Gerber van der Graaf <gerber_graaf@users.sf.net>
#-------------------------------------------------------------------------------
# License
#   This file is part of FreeFOAM.
#
#   FreeFOAM is free software; you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published by the
#   Free Software Foundation; either version 2 of the License, or (at your
#   option) any later version.
#
#   FreeFOAM is distributed in the hope that it will be useful, but WITHOUT
#   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
#   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
#   for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with FreeFOAM; if not, write to the Free Software Foundation,
#   Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
#-------------------------------------------------------------------------------

# - Find execinfo (i.e. backtrace() and backtrace_symbols())
#
# This module looks for execinfo support and defines the following values
#  EXECINFO_FOUND          TRUE if execinfo has been found
#  EXECINFO_INCLUDE_DIRS   include path for execinfo
#  EXECINFO_LIBRARIES      libraries to link against (if required)

# take extra care on APPLE, the result depends on the sysroot and the deployment target
if( APPLE )
  ff_detect_changed_value( __find_execinfo_CMAKE_OSX_SYSROOT_CHANGED CMAKE_OSX_SYSROOT
    EXECINFO_INCLUDE_DIR )
  ff_detect_changed_value( __find_execinfo_CMAKE_OSX_DEPLOYMENT_TARGET_CHANGED CMAKE_OSX_DEPLOYMENT_TARGET
    EXECINFO_INCLUDE_DIR )
  set( __find_execinfo_FIND_PATH_OPTS PATHS
    ${CMAKE_OSX_SYSROOT}/usr/include
    NO_DEFAULT_PATH )
else( APPLE )
  set( __find_execinfo_FIND_PATH_OPTS )
endif( APPLE )

find_path( EXECINFO_INCLUDE_DIR
  NAMES execinfo.h
  ${__find_execinfo_FIND_PATH_OPTS}
  )
mark_as_advanced( EXECINFO_INCLUDE_DIR )

set( EXECINFO_INCLUDE_DIRS ${EXECINFO_INCLUDE_DIR} )

# now check whether libexecinfo is required
set( __find_execinfo_LINK_LIBRARIES ${LINK_LIBRARIES} )
set( __find_execinfo_test_SRC
  "#include <execinfo.h>
  int main() {
  void* callstack[128];
  int frames = backtrace(callstack, 128);
  char** strs = backtrace_symbols(callstack, frames);
  free(strs);
  return 0;
  }\n"
  )
set( __find_execinfo_COMPILES FALSE )
set( __find_execinfo_REQUIRED_INCLUDE_DIRS "-DINCLUDE_DIRECTORIES=${EXECINFO_INCLUDE_DIR}" )
foreach( __find_execinfo_with_lib FALSE TRUE )
  if( NOT __find_execinfo_COMPILES )
    if( __find_execinfo_with_lib )
      find_library( EXECINFO_LIBRARY NAMES execinfo )
      mark_as_advanced( EXECINFO_LIBRARY )
      set( __find_execinfo_REQUIRED_LIBS "-DLINK_LIBRARIES=${EXECINFO_LIBRARY}" )
    else( __find_execinfo_with_lib )
      set( __find_execinfo_REQUIRED_LIBS )
    endif( __find_execinfo_with_lib )
    file( WRITE
      ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/execinfo_test.c
      "${__find_execinfo_test_SRC}"
      )
    try_compile(  __find_execinfo_COMPILES ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp
      ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/execinfo_test.c
      CMAKE_FLAGS "${__find_execinfo_REQUIRED_LIBS}" "${__find_execinfo_REQUIRED_INCLUDE_DIRS}"
      )
  endif( NOT __find_execinfo_COMPILES )
endforeach( __find_execinfo_with_lib )
# if the test compiled and we found a library, add it...
if( __find_execinfo_COMPILES AND EXECINFO_LIBRARY )
  set( EXECINFO_LIBRARIES ${EXECINFO_LIBRARY} )
endif( __find_execinfo_COMPILES AND EXECINFO_LIBRARY )

# handle standard stuff
include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( execinfo
  DEFAULT_MSG
  EXECINFO_INCLUDE_DIR
  __find_execinfo_COMPILES
  )

# ------------------------- vim: set sw=2 sts=2 et: --------------- end-of-file

