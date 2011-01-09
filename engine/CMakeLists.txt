if (HAT_USE_OPENAL)
	message(STATUS "- openal.............. ENABLED")
	find_package(OpenAL REQUIRED)
	if(OPENAL_FOUND)
		message(STATUS "- openal include...... ${OPENAL_INCLUDE_DIR}")
		message(STATUS "- openal library...... ${OPENAL_LIBRARY}")
		include_directories(${OPENAL_INCLUDE_DIR})
		set(DEPENDENCIES ${DEPENDENCIES} ${OPENAL_LIBRARY})
	endif(OPENAL_FOUND)
else()
	message(STATUS "- openal.............. DISABLED")
endif()

# Vorbis
set(VORBIS_INCLUDE_DIR ${ENGINE_DEP_DIR}/vorbis/include)
set(VORBIS_SRC_DIR ${ENGINE_DEP_DIR}/vorbis/lib)

# Theora
set(THEORA_INCLUDE_DIR ${ENGINE_DEP_DIR}/theora/include)
set(THEORA_SRC_DIR ${ENGINE_DEP_DIR}/theora/lib/dec)

# Speex
set(SPEEX_INCLUDE_DIR ${ENGINE_DEP_DIR}/speex/include)
set(SPEEX_SRC_DIR ${ENGINE_DEP_DIR}/speex)

# Ogg
set(OGG_INCLUDE_DIR ${ENGINE_DEP_DIR}/ogg/include)
set(OGG_SRC_DIR ${ENGINE_DEP_DIR}/ogg/src)

# ZLIB
set(ZLIB_INCLUDE_DIR ${ENGINE_DEP_DIR}/zlib)
set(ZLIB_SRC_DIR ${ENGINE_DEP_DIR}/zlib)

if(WIN32)
	set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} 
		${ENGINE_DEP_DIR}/sdl/lib
		${ENGINE_DEP_DIR}/curl/lib)
	set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH}
		${ENGINE_DEP_DIR}/sdl/include
		${ENGINE_DEP_DIR}/curl/include)
endif(WIN32)

find_package(SDL REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenGL REQUIRED)
find_package(V8 REQUIRED)

if(SDL_FOUND)
	if(WIN32)
		message(STATUS "- using local sdl..... TRUE")
	else()
		message(STATUS "- using local sdl..... FALSE")
	endif(WIN32)
	message(STATUS "- sdl include......... ${SDL_INCLUDE_DIR}")
	message(STATUS "- sdl library......... ${SDL_LIBRARY}")
	set(DEPENDENCIES ${DEPENDENCIES} ${SDL_LIBRARY})
	include_directories(${SDL_INCLUDE_DIR})
endif(SDL_FOUND)

if(CURL_FOUND)
	if(WIN32)
		message(STATUS "- using local curl.... TRUE")
	else()
		message(STATUS "- using local curl.... FALSE")
	endif(WIN32)
	message(STATUS "- curl include........ ${CURL_INCLUDE_DIR}")
	message(STATUS "- curl library........ ${CURL_LIBRARY}")
	set(DEPENDENCIES ${DEPENDENCIES} ${CURL_LIBRARY})
	include_directories(${CURL_INCLUDE_DIR})
endif(CURL_FOUND)

if(OPENGL_FOUND)
	message(STATUS "- opengl include...... ${OPENGL_INCLUDE_DIR}")
	message(STATUS "- opengl library...... ${OPENGL_LIBRARY}")
	set(DEPENDENCIES ${DEPENDENCIES} ${OPENGL_LIBRARY})
	include_directories(${OPENGL_INCLUDE_DIR})
endif(OPENGL_FOUND)

if(V8_FOUND)
	message(STATUS "- v8 include.......... ${V8_INCLUDE_DIR}")
	message(STATUS "- v8 library.......... ${V8_LIBRARY}")
	set(DEPENDENCIES ${DEPENDENCIES} ${V8_LIBRARY})
	include_directories(${V8_INCLUDE_DIR})
endif(V8_FOUND)

# Source lists
set(SERVER_INCLUDES bg_public.h g_public.h)
set(CLIENT_INCLUDES cg_public.h)
set(GUI_INCLUDES ui_public.h)

set(ENGINE_INCLUDES
	client.h
	cl_curl.h
	cm_local.h
	cm_polylib.h
	cm_public.h
	engine_tests.h
	keycodes.h
	keys.h
	qal.h
	qcommon.h
	qfiles.h
	q_platform.h
	q_print.h
	q_shared.h
	server.h
	snd_codec.h
	snd_local.h
	snd_public.h
	surfaceflags.h
	sys_loadlib.h
	sys_local.h
	unzip.h
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

set(VORBIS_SRC
	analysis.c
	bitrate.c
	block.c
	codebook.c
	envelope.c
	floor0.c
	floor1.c
	info.c
	lookup.c
	lpc.c
	lsp.c
	mapping0.c
	mdct.c
	psy.c
	registry.c
	res0.c
	sharedbook.c
	smallft.c
	synthesis.c
	vorbisfile.c
	window.c
)

set(THEORA_SRC
	apiwrapper.c
	bitpack.c
	decapiwrapper.c
	decinfo.c
	decode.c
	dequant.c
	fragment.c
	huffdec.c
	idct.c
	internal.c
	quant.c
	state.c
	thinfo.c
	x86/mmxfrag.c
	x86/mmxidct.c
	x86/mmxstate.c
	x86/x86state.c
)

set(SPEEX_SRC
	bits.c
	buffer.c
	cb_search.c
	exc_10_16_table.c
	exc_10_32_table.c
	exc_20_32_table.c
	exc_5_256_table.c
	exc_5_64_table.c
	exc_8_128_table.c
	fftwrap.c
	filterbank.c
	filters.c
	gain_table.c
	gain_table_lbr.c
	hexc_10_32_table.c
	hexc_table.c
	high_lsp_tables.c
	jitter.c
	kiss_fft.c
	kiss_fftr.c
	lsp_tables_nb.c
	ltp.c
	mdf.c
	modes.c
	modes_wb.c
	nb_celp.c
	preprocess.c
	quant_lsp.c
	resample.c
	sb_celp.c
	smallft.c
	speex.c
	speex_callbacks.c
	speex_header.c
	speex_lpc.c
	speex_lsp.c
	speex_window.c
	stereo.c
	vbr.c
	vq.c
)

set(OGG_SRC bitwise.c framing.c)

set(ENGINE_SRC
	cin_ogm.c
	cl_avi.c
	cl_cgame.c
	cl_cgame_java.c
	cl_cin.c
	cl_console.c
	cl_curl.c
	cl_input.c
	cl_keys.c
	cl_main.c
	cl_net_chan.c
	cl_parse.c
	cl_scrn.c
	cl_ui.c
	cl_ui_java.c
	cmd.c
	cm_load.c
	cm_patch.c
	cm_polylib.c
	cm_test.c
	cm_trace.c
	cm_trisoup.c
	common.c
	con_log.c
	cvar.c
	files.c
	engine_tests.cpp
	huffman.c
	md4.c
	md5.c
	msg.c
	net_chan.c
	net_ip.c
	parse.c
	qal.c
	q_math.c
	q_shared.c
	sdl_input.c
	sdl_snd.c
	snd_codec.c
	snd_codec_ogg.c
	snd_codec_wav.c
	snd_dma.c
	snd_main.c
	snd_mem.c
	snd_mix.c
	snd_openal.c
	sv_bot.c
	sv_ccmds.c
	sv_client.c
	sv_game.c
	sv_game_java.c
	sv_init.c
	sv_main.c
	sv_net_chan.c
	sv_snapshot.c
	sv_world.c
	sys_main.c
	v8/type_conversion.cpp
	unzip.c
	vm.c
	vm_java.c
)

# These are separated because it is easier to manage in MSVC

set(JAVASCRIPT_WEAPON_INCLUDES
	${ENGINE_HAT_DIR}/javascript/weapon/class.hpp
	${ENGINE_HAT_DIR}/javascript/weapon/interface.hpp
	${ENGINE_HAT_DIR}/javascript/weapon/manager.hpp
	${ENGINE_HAT_DIR}/javascript/weapon.hpp
	${ENGINE_HAT_DIR}/javascript/weapon.h
)

set(JAVASCRIPT_WEAPON_SRC
	${ENGINE_SRC_DIR}/javascript/weapon/class.cpp
	${ENGINE_SRC_DIR}/javascript/weapon/manager.cpp
	${ENGINE_SRC_DIR}/javascript/weapon/tools.cpp
)

if(APPLE)
	message(STATUS "- system type......... APPLE")
	message(STATUS "- needs iokit......... TRUE")

	find_package(IOKit REQUIRED)
	if (IOKIT_FOUND)
		message(STATUS "- needs iokit......... ${IOKIT_LIBRARY}")
	else()
		message(STATUS "- needs iokit......... FAILED")
	endif(IOKIT_FOUND)

	set(SPECIFIC_LIBRARIES ${IOKIT_LIBRARY})
	set(APPLE_SRC sys_unix.c con_passive.c sys_osx.m)
	set(ENGINE_SRC ${ENGINE_SRC} ${APPLE_SRC})
	add_definitions(-D__MACOSX__ -DMACOS_X)
	message(STATUS "- additional files.... ${APPLE_SRC}")
elseif(UNIX)
	message(STATUS "- system type......... UNIX")
	set(UNIX_SRC sys_unix.c con_passive.c)
	set(ENGINE_SRC ${ENGINE_SRC} ${UNIX_SRC})
	message(STATUS "- additional files.... ${UNIX_SRC}")
elseif(WIN32)
	message(STATUS "- system type......... WIN32")
	set(SPECIFIC_LINKER_FLAGS "/FIXED:NO /DYNAMICBASE:NO /LARGEADDRESSAWARE")
	set(WIN32_SRC con_win32.c sys_win32.c)
	set(ENGINE_SRC ${ENGINE_SRC} ${WIN32_SRC})
	set(SPECIFIC_LIBRARIES winmm.lib ws2_32.lib PSAPI.lib user32.lib)
	set(RESOURCE_SRC win_resource.h win_resource.rc rr.ico)
	message(STATUS "- additional files.... ${SPECIFIC_SRC} ${RESOURCE_SRC}")
	message(STATUS "- additional libraries ${SPECIFIC_LIBRARIES}")
else()
	message(FATAL_ERROR "Unsupported operating system.")
endif()

generate_hat_sources()
generate_external_sources(THEORA OGG VORBIS SPEEX ZLIB)

# Manually specify structure for these
set(SRC_ALL ${SRC_ALL} ${JAVASCRIPT_WEAPON_SRC} ${JAVASCRIPT_WEAPON_INCLUDES})
source_group("JavaScript\\Weapon\\Header Files" 
	FILES ${JAVASCRIPT_WEAPON_INCLUDES})
source_group("JavaScript\\Weapon\\Source Files"
	FILES ${JAVASCRIPT_WEAPON_SRC})

# Setup the executable
add_executable(engine ${SRC_ALL})
add_executable(dedicated ${SRC_ALL})

target_link_libraries(engine ${SPECIFIC_LIBRARIES} ${DEPENDENCIES})
target_link_libraries(dedicated ${SPECIFIC_LIBRARIES} ${DEPENDENCIES})

set_target_properties(dedicated PROPERTIES COMPILE_FLAGS "-DDEDICATED -DPARANOID")

# Strangely enough, I need this because of Mac OS X.
if(DEFINED SPECIFIC_LINKER_FLAGS)
	set_target_properties(engine PROPERTIES 
		LINK_FLAGS ${SPECIFIC_LINKER_FLAGS})
	set_target_properties(dedicated PROPERTIES 
		LINK_FLAGS ${SPECIFIC_LINKER_FLAGS})
endif(DEFINED SPECIFIC_LINKER_FLAGS)

add_definitions(
	-DUSE_CURL
	-DBUILD_FREETYPE
	-DFT2_BUILD_LIBRARY
	-DSTANDALONE
	-DUSE_CODEC_VORBIS
	-DUSE_VOIP
	-DHAVE_CONFIG_H
	-DUSE_CIN_THEORA
	-DUSE_ALLOCA
	-DFLOATING_POINT
)

install(TARGETS engine dedicated DESTINATION ${CMAKE_INSTALL_PREFIX})
