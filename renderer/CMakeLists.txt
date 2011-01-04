# Rogue Reborn Engine (Hat)
cmake_minimum_required(VERSION 2.8)

# Options
option(HAT_EXPAND_DEPENDENCIES "If enabled, Visual Studio will show all the dependencies broken apart into their respective libraries." FALSE)
option(HAT_EXPAND_PLATFORM "If enabled, Visual Studio will show all the operating-system specific dependencies in their own folders." FALSE)

# Setup our paths
# Server
set(SERVER_INCLUDE_TOPLEVEL_DIR "../server/include")
set(SERVER_INCLUDE_SERVER_DIR "${SERVER_INCLUDE_TOPLEVEL_DIR}/hat/server")

# Client
set(CLIENT_INCLUDE_TOPLEVEL_DIR "../client/include")
set(CLIENT_INCLUDE_CLIENT_DIR "${CLIENT_INCLUDE_TOPLEVEL_DIR}/hat/client")

# Gui
set(GUI_INCLUDE_TOPLEVEL_DIR "../gui/include")
set(GUI_INCLUDE_GUI_DIR "${GUI_INCLUDE_TOPLEVEL_DIR}/hat/gui")

# Engine 
set(ENGINE_INCLUDE_TOPLEVEL_DIR "../engine/include")
set(ENGINE_SRC_DIR "../engine/src")
set(ENGINE_INCLUDE_ENGINE_DIR "${ENGINE_INCLUDE_TOPLEVEL_DIR}/hat/engine")

# Dependency top-level
set(ENGINE_DEP_TOPLEVEL_DIR "../engine/dep")
set(RENDERER_DEP_TOPLEVEL_DIR "dep")

# Zlib
set(ENGINE_DEP_ZLIB_DIR "${ENGINE_DEP_TOPLEVEL_DIR}/zlib")
set(ENGINE_DEP_ZLIB_INCLUDE_DIR "${ENGINE_DEP_TOPLEVEL_DIR}/zlib")
set(ENGINE_DEP_ZLIB_SRC_DIR "${ENGINE_DEP_TOPLEVEL_DIR}/zlib")

# Freetype
set(RENDERER_DEP_FREETYPE_DIR "${RENDERER_DEP_TOPLEVEL_DIR}/freetype")
set(RENDERER_DEP_FREETYPE_SRC_DIR "${RENDERER_DEP_FREETYPE_DIR}/src")
set(RENDERER_DEP_FREETYPE_INCLUDE_DIR "${RENDERER_DEP_FREETYPE_DIR}/include")

# JPEG
set(RENDERER_DEP_JPEG_DIR "${RENDERER_DEP_TOPLEVEL_DIR}/jpeg-6")
set(RENDERER_DEP_JPEG_INCLUDE_DIR "${RENDERER_DEP_JPEG_DIR}")
set(RENDERER_DEP_JPEG_SRC_DIR "${RENDERER_DEP_JPEG_DIR}")

# OpenEXR
set(RENDERER_DEP_OPENEXR_DIR "${RENDERER_DEP_TOPLEVEL_DIR}/openexr")
set(RENDERER_DEP_OPENEXR_INCLUDE_DIR "${RENDERER_DEP_OPENEXR_DIR}")
set(RENDERER_DEP_OPENEXR_SRC_DIR "${RENDERER_DEP_OPENEXR_DIR}")

# PNG 
set(RENDERER_DEP_PNG_DIR "${RENDERER_DEP_TOPLEVEL_DIR}/png")
set(RENDERER_DEP_PNG_INCLUDE_DIR "${RENDERER_DEP_PNG_DIR}")
set(RENDERER_DEP_PNG_SRC_DIR "${RENDERER_DEP_PNG_DIR}")

# Various includes
set(RENDERER_INCLUDE_TOPLEVEL_DIR "include")
set(RENDERER_INCLUDE_RENDERER_DIR "${RENDERER_INCLUDE_TOPLEVEL_DIR}/hat/renderer")
set(RENDERER_SRC_DIR "src")

# SDL is configuration-specific and Curl is a pain in the ass
# So, we have prebuilt binaries/libraries for Windows
if(WIN32)
	set(ENGINE_DEP_SDL_DIR "${ENGINE_DEP_TOPLEVEL_DIR}/sdl")
	set(ENGINE_DEP_SDL_INCLUDE_DIR "${ENGINE_DEP_SDL_DIR}/include")
	set(ENGINE_DEP_SDL_LIB_DIR "${ENGINE_DEP_SDL_DIR}/lib")

	set(CMAKE_LIBRARY_PATH 
		${CMAKE_LIBRARY_PATH} 
		${ENGINE_DEP_SDL_LIB_DIR}
	)

	# External includes
	set(CMAKE_INCLUDE_PATH
		${CMAKE_INCLUDE_PATH}
		${ENGINE_DEP_SDL_INCLUDE_DIR}
	)
elseif(APPLE)
	message(STATUS "Looking for Cocoa...")
	find_package(Cocoa)
	if (NOT COCOA_FOUND)
		message(FATAL_ERROR "Cocoa could not be found!")
	else()
		message(STATUS "Cocoa was found: ${COCOA_LIBRARY}")
	endif(NOT COCOA_FOUND)
	set(OS_SPECIFIC_LIBRARY ${COCOA_LIBRARY})
endif()


# These packages are actually needed and I rather not put them into the build
# system. I recommended OpenAL to be openal-soft.
find_package(SDL REQUIRED)

set(RENDERER_SRC_HEADER_LIST
	# Renderer headers
	"${RENDERER_INCLUDE_RENDERER_DIR}/qgl.h"
	"${RENDERER_INCLUDE_RENDERER_DIR}/tr_local.h"
	"${RENDERER_INCLUDE_RENDERER_DIR}/tr_public.h"
	"${RENDERER_INCLUDE_RENDERER_DIR}/tr_types.h"
)	


set(ENGINE_SRC_HEADER_LIST
	# Engine headers
	"${ENGINE_INCLUDE_ENGINE_DIR}/q_platform.h"
	"${ENGINE_INCLUDE_ENGINE_DIR}/q_shared.h"
)

set(RENDERER_SRC_LIST
	# Renderer sources
	"${RENDERER_INCLUDE_RENDERER_DIR}/renderer.def"
	"${RENDERER_SRC_DIR}/qgl.c"
	"${RENDERER_SRC_DIR}/tr_animation.c"
	"${RENDERER_SRC_DIR}/tr_backend.c"
	"${RENDERER_SRC_DIR}/tr_bsp.c"
	"${RENDERER_SRC_DIR}/tr_cmds.c"
	"${RENDERER_SRC_DIR}/tr_curve.c"
	"${RENDERER_SRC_DIR}/tr_fbo.c"
	"${RENDERER_SRC_DIR}/tr_flares.c"
	"${RENDERER_SRC_DIR}/tr_font.c"
	"${RENDERER_SRC_DIR}/tr_image.c"
	"${RENDERER_SRC_DIR}/tr_image_dds.c"
	"${RENDERER_SRC_DIR}/tr_image_exr.cpp"
	"${RENDERER_SRC_DIR}/tr_image_jpg.c"
	"${RENDERER_SRC_DIR}/tr_image_png.c"
	"${RENDERER_SRC_DIR}/tr_image_tga.c"
	"${RENDERER_SRC_DIR}/tr_init.cpp"
	"${RENDERER_SRC_DIR}/tr_light.c"
	"${RENDERER_SRC_DIR}/tr_main.c"
	"${RENDERER_SRC_DIR}/tr_marks.c"
	"${RENDERER_SRC_DIR}/tr_mesh.c"
	"${RENDERER_SRC_DIR}/tr_model.c"
	"${RENDERER_SRC_DIR}/tr_noise.c"
	"${RENDERER_SRC_DIR}/tr_scene.c"
	"${RENDERER_SRC_DIR}/tr_shade.c"
	"${RENDERER_SRC_DIR}/tr_shader.c"
	"${RENDERER_SRC_DIR}/tr_shade_calc.c"
	"${RENDERER_SRC_DIR}/tr_shadows.c"
	"${RENDERER_SRC_DIR}/tr_sky.c"
	"${RENDERER_SRC_DIR}/tr_surface.c"
	"${RENDERER_SRC_DIR}/tr_vbo.c"
	"${RENDERER_SRC_DIR}/tr_world.c"
)

set(ENGINE_SRC_LIST
	"${ENGINE_SRC_DIR}/q_math.c"
	"${ENGINE_SRC_DIR}/q_shared.c"
	"${ENGINE_SRC_DIR}/sdl_gamma.c"
	"${ENGINE_SRC_DIR}/sdl_glimp.c"
)


set(ENGINE_SRC_ZLIB_LIST
	# Zlib source files
	"${ENGINE_DEP_ZLIB_DIR}/adler32.c"
	"${ENGINE_DEP_ZLIB_DIR}/compress.c"
	"${ENGINE_DEP_ZLIB_DIR}/crc32.c"
	"${ENGINE_DEP_ZLIB_DIR}/deflate.c"
	"${ENGINE_DEP_ZLIB_DIR}/gzio.c"
	"${ENGINE_DEP_ZLIB_DIR}/infback.c"
	"${ENGINE_DEP_ZLIB_DIR}/inffast.c"
	"${ENGINE_DEP_ZLIB_DIR}/inflate.c"
	"${ENGINE_DEP_ZLIB_DIR}/inftrees.c"
	"${ENGINE_DEP_ZLIB_DIR}/ioapi.c"
	"${ENGINE_DEP_ZLIB_DIR}/trees.c"
	"${ENGINE_DEP_ZLIB_DIR}/uncompr.c"
	"${ENGINE_DEP_ZLIB_DIR}/zutil.c"
)

set(RENDERER_SRC_FREETYPE_LIST
	# Freetype source files
	# Modules
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/base/ftbbox.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/base/ftmm.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/base/ftpfr.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/base/ftsynth.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/base/fttype1.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/base/ftwinfnt.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/pcf/pcf.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/pfr/pfr.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/psaux/psaux.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/pshinter/pshinter.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/psnames/psmodule.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/raster/raster.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/sfnt/sfnt.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/truetype/truetype.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/type1/type1.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/cid/type1cid.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/type42/type42.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/winfonts/winfnt.c"
	# Source
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/autofit/autofit.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/bdf/bdf.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/cff/cff.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/base/ftbase.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/base/ftbitmap.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/cache/ftcache.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/base/ftdebug.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/base/ftgasp.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/base/ftglyph.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/gzip/ftgzip.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/base/ftinit.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/lzw/ftlzw.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/base/ftstroke.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/base/ftsystem.c"
	"${RENDERER_DEP_FREETYPE_SRC_DIR}/smooth/smooth.c"
)

set(RENDERER_SRC_JPEG_LIST
	# JPEG
	"${RENDERER_DEP_JPEG_SRC_DIR}/jcapimin.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jccoefct.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jccolor.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jcdctmgr.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jchuff.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jcinit.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jcmainct.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jcmarker.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jcmaster.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jcomapi.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jcparam.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jcphuff.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jcprepct.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jcsample.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jctrans.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jdapimin.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jdapistd.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jdatadst.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jdatasrc.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jdcoefct.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jdcolor.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jddctmgr.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jdhuff.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jdinput.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jdmainct.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jdmarker.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jdmaster.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jdpostct.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jdsample.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jdtrans.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jerror.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jfdctflt.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jidctflt.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jmemmgr.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jmemnobs.c"
	"${RENDERER_DEP_JPEG_SRC_DIR}/jutils.c"
)

set(RENDERER_SRC_OPENEXR_LIST
	# OpenEXR
	"${RENDERER_DEP_OPENEXR_SRC_DIR}/half.cpp"
)

set(RENDERER_SRC_PNG_LIST
	# PNG
	"${RENDERER_DEP_PNG_SRC_DIR}/png.c"
	"${RENDERER_DEP_PNG_SRC_DIR}/pngerror.c"
	"${RENDERER_DEP_PNG_SRC_DIR}/pnggccrd.c"
	"${RENDERER_DEP_PNG_SRC_DIR}/pngget.c"
	"${RENDERER_DEP_PNG_SRC_DIR}/pngmem.c"
	"${RENDERER_DEP_PNG_SRC_DIR}/pngpread.c"
	"${RENDERER_DEP_PNG_SRC_DIR}/pngread.c"
	"${RENDERER_DEP_PNG_SRC_DIR}/pngrio.c"
	"${RENDERER_DEP_PNG_SRC_DIR}/pngrtran.c"
	"${RENDERER_DEP_PNG_SRC_DIR}/pngrutil.c"
	"${RENDERER_DEP_PNG_SRC_DIR}/pngset.c"
	"${RENDERER_DEP_PNG_SRC_DIR}/pngtest.c"
	"${RENDERER_DEP_PNG_SRC_DIR}/pngtrans.c"
	"${RENDERER_DEP_PNG_SRC_DIR}/pngvcrd.c"
	"${RENDERER_DEP_PNG_SRC_DIR}/pngwio.c"
	"${RENDERER_DEP_PNG_SRC_DIR}/pngwrite.c"
	"${RENDERER_DEP_PNG_SRC_DIR}/pngwtran.c"
	"${RENDERER_DEP_PNG_SRC_DIR}/pngwutil.c"
)


# Define the final source list
set(RENDERER_SRC_ALL_LIST
	${RENDERER_SRC_HEADER_LIST}
	${ENGINE_SRC_HEADER_LIST}
	${RENDERER_SRC_LIST}
	${ENGINE_SRC_LIST}
	${ENGINE_SRC_ZLIB_LIST}
	${RENDERER_SRC_FREETYPE_LIST}
	${RENDERER_SRC_JPEG_LIST}
	${RENDERER_SRC_OPENEXR_LIST}
	${RENDERER_SRC_PNG_LIST}
)

# Project include
include_directories(
	"${ENGINE_INCLUDE_TOPLEVEL_DIR}"
	"${RENDERER_INCLUDE_TOPLEVEL_DIR}"
	"${SERVER_INCLUDE_TOPLEVEL_DIR}"
	"${CLIENT_INCLUDE_TOPLEVEL_DIR}"
	"${GUI_INCLUDE_TOPLEVEL_DIR}"
	"${ENGINE_DEP_ZLIB_INCLUDE_DIR}"
	"${RENDERER_DEP_FREETYPE_INCLUDE_DIR}"
	"${RENDERER_DEP_JPEG_INCLUDE_DIR}"
	"${RENDERER_DEP_OPENEXR_INCLUDE_DIR}"
	"${RENDERER_DEP_PNG_INCLUDE_DIR}"
	"${SDL_INCLUDE_DIR}"
)

# Group the source code
source_group("Header Files" FILES ${RENDERER_SRC_HEADER_LIST})
source_group("Source Files" FILES ${RENDERER_SRC_LIST} ${ENGINE_SRC_LIST})
if(${HAT_EXPAND_DEPENDENCIES})
	source_group("(library) Freetype" FILES ${RENDERER_SRC_FREETYPE_LIST})
	source_group("(library) JPEG" FILES ${RENDERER_SRC_JPEG_LIST})
	source_group("(library) Zlib" FILES ${ENGINE_SRC_ZLIB_LIST})
	source_group("(library) OpenEXR" FILES ${ENGINE_SRC_OPENEXR_LIST})
	source_group("(library) PNG" FILES ${ENGINE_SRC_PNG_LIST})
else()
	source_group("External Libraries"
		FILES
		${ENGINE_SRC_ZLIB_LIST}
		${RENDERER_SRC_FREETYPE_LIST}
		${RENDERER_SRC_JPEG_LIST}
		${RENDERER_SRC_OPENEXR_LIST}
		${RENDERER_SRC_PNG_LIST}
	)
endif()

# Setup the library
add_library(renderer SHARED ${RENDERER_SRC_ALL_LIST})
target_link_libraries(renderer ${SDL_LIBRARY} ${OS_SPECIFIC_LIBRARY})
add_definitions(
	-DBUILD_FREETYPE
	-DFT2_BUILD_LIBRARY
)

set_target_properties(renderer PROPERTIES PREFIX "")

if(WIN32)
set_target_properties(renderer PROPERTIES LINK_FLAGS
	"/DYNAMICBASE:NO"
)
endif()

install(TARGETS renderer DESTINATION ${CMAKE_INSTALL_PREFIX})
