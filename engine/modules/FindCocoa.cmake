# Copyright (c) 2011, Justin Bruce Van Horne <justinvh@gmail.com>

include(CMakeFindFrameworks)

cmake_find_frameworks(Cocoa)

if (Cocoa_FRAMEWORKS)
  set(COCOA_LIBRARY "-framework Cocoa -framework OpenGL" CACHE FILEPATH "Cocoa framework" FORCE)
  set(COCOA_FOUND TRUE)
endif (Cocoa_FRAMEWORKS)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Cocoa DEFAULT_MSG COCOA_LIBRARY)
