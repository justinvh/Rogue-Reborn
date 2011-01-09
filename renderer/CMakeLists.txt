# Zlib
set(ZLIB_INCLUDE_DIR ${ENGINE_DEP_DIR}/zlib)
set(ZLIB_SRC_DIR ${ENGINE_DEP_DIR}/zlib)

# Freetype
set(FREETYPE_INCLUDE_DIR ${RENDERER_DEP_DIR}/freetype/include)
set(FREETYPE_SRC_DIR ${RENDERER_DEP_DIR}/freetype/src)

# JPEG
set(JPEG_INCLUDE_DIR ${RENDERER_DEP_DIR}/jpeg-6)
set(JPEG_SRC_DIR ${RENDERER_DEP_DIR}/jpeg-6)

# OpenEXR
set(OPENEXR_INCLUDE_DIR ${RENDERER_DEP_DIR}/openexr)
set(OPENEXR_SRC_DIR ${RENDERER_DEP_DIR}/openexr)

# PNG 
set(PNG_INCLUDE_DIR ${RENDERER_DEP_DIR}/png)
set(PNG_SRC_DIR ${RENDERER_DEP_DIR}/png)

if(WIN32)
	set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} 
		${ENGINE_DEP_DIR}/sdl/lib)
	set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH}
		${ENGINE_DEP_DIR}/sdl/include)
elseif(APPLE)
	message(STATUS "- cocoa required......")
	find_package(Cocoa)
	if (NOT COCOA_FOUND)
		message(FATAL_ERROR "x Cocoa was not found.")
	else()
		message(STATUS "- cocoa found......... ${COCOA_LIBRARY}")
	endif(NOT COCOA_FOUND)
	set(SPECIFIC_LIBRARIES ${COCOA_LIBRARY})
endif()

find_package(SDL REQUIRED)

if(SDL_FOUND)
	if(WIN32)
		message(STATUS "- using local sdl..... TRUE")
	else()
		message(STATUS "- using local sdl..... FALSE")
	endif(WIN32)
	message(STATUS "- sdl include......... ${SDL_INCLUDE_DIR}")
	message(STATUS "- sdl library......... ${SDL_LIBRARY}")
	include_directories(${SDL_INCLUDE_DIR})
endif(SDL_FOUND)

# Engine includes and sources
set(ENGINE_INCLUDES q_platform.h q_shared.h)
set(ENGINE_SRC q_math.c q_shared.c sdl_gamma.c sdl_glimp.c)

# Renderer include and sources
set(RENDERER_INCLUDES qgl.h tr_local.h tr_public.h tr_types.h renderer.def)
set(RENDERER_SRC
	qgl.c
	tr_animation.c
	tr_backend.c
	tr_bsp.c
	tr_cmds.c
	tr_curve.c
	tr_fbo.c
	tr_flares.c
	tr_font.c
	tr_image.c
	tr_image_dds.c
	tr_image_exr.cpp
	tr_image_jpg.c
	tr_image_png.c
	tr_image_tga.c
	tr_init.cpp
	tr_light.c
	tr_main.c
	tr_marks.c
	tr_mesh.c
	tr_model.c
	tr_noise.c
	tr_scene.c
	tr_shade.c
	tr_shader.c
	tr_shade_calc.c
	tr_shadows.c
	tr_sky.c
	tr_surface.c
	tr_vbo.c
	tr_world.c
)

set(ZLIB_SRC
	adler32.c
	compress.c
	crc32.c
	deflate.c
	gzio.c
	infback.c
	inffast.c
	inflate.c
	inftrees.c
	ioapi.c
	trees.c
	uncompr.c
	zutil.c
)

set(FREETYPE_SRC
	base/ftbbox.c
	base/ftmm.c
	base/ftpfr.c
	base/ftsynth.c
	base/fttype1.c
	base/ftwinfnt.c
	pcf/pcf.c
	pfr/pfr.c
	psaux/psaux.c
	pshinter/pshinter.c
	psnames/psmodule.c
	raster/raster.c
	sfnt/sfnt.c
	truetype/truetype.c
	type1/type1.c
	cid/type1cid.c
	type42/type42.c
	winfonts/winfnt.c
	autofit/autofit.c
	bdf/bdf.c
	cff/cff.c
	base/ftbase.c
	base/ftbitmap.c
	cache/ftcache.c
	base/ftdebug.c
	base/ftgasp.c
	base/ftglyph.c
	gzip/ftgzip.c
	base/ftinit.c
	lzw/ftlzw.c
	base/ftstroke.c
	base/ftsystem.c
	smooth/smooth.c
)

set(JPEG_SRC
	jcapimin.c
	jccoefct.c
	jccolor.c
	jcdctmgr.c
	jchuff.c
	jcinit.c
	jcmainct.c
	jcmarker.c
	jcmaster.c
	jcomapi.c
	jcparam.c
	jcphuff.c
	jcprepct.c
	jcsample.c
	jctrans.c
	jdapimin.c
	jdapistd.c
	jdatadst.c
	jdatasrc.c
	jdcoefct.c
	jdcolor.c
	jddctmgr.c
	jdhuff.c
	jdinput.c
	jdmainct.c
	jdmarker.c
	jdmaster.c
	jdpostct.c
	jdsample.c
	jdtrans.c
	jerror.c
	jfdctflt.c
	jidctflt.c
	jmemmgr.c
	jmemnobs.c
	jutils.c
)

set(OPENEXR_SRC half.cpp)

set(PNG_SRC
	png.c
	pngerror.c
	pnggccrd.c
	pngget.c
	pngmem.c
	pngpread.c
	pngread.c
	pngrio.c
	pngrtran.c
	pngrutil.c
	pngset.c
	pngtest.c
	pngtrans.c
	pngvcrd.c
	pngwio.c
	pngwrite.c
	pngwtran.c
	pngwutil.c
)

generate_hat_sources()
generate_external_sources(ZLIB JPEG OPENEXR PNG FREETYPE)

# Setup the library
add_library(renderer SHARED ${SRC_ALL})
target_link_libraries(renderer ${SDL_LIBRARY} ${SPECIFIC_LIBRARIES})
set_target_properties(renderer PROPERTIES PREFIX "")
add_definitions(-DBUILD_FREETYPE -DFT2_BUILD_LIBRARY)

# Installation
install(TARGETS renderer DESTINATION ${CMAKE_INSTALL_PREFIX})
