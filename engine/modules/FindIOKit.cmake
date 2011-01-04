# Copyright (c) 2009, Harald Fernegel <harry@kdevelop.org)

include(CMakeFindFrameworks)

cmake_find_frameworks(IOKit)
cmake_find_frameworks(CoreFoundation)

if (IOKit_FRAMEWORKS)
  set(IOKIT_LIBRARY "-framework IOKit -framework CoreFoundation" CACHE FILEPATH "IOKit framework" FORCE)
  set(IOKIT_FOUND TRUE)
endif (IOKit_FRAMEWORKS)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(IOKit DEFAULT_MSG IOKIT_LIBRARY)
