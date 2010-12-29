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

# Renderer
set(RENDERER_INCLUDE_TOPLEVEL_DIR "../renderer/include")

# Dependency top-level
set(ENGINE_DEP_TOPLEVEL_DIR "dep")
set(ENGINE_DEP_ZLIB_DIR "${ENGINE_DEP_TOPLEVEL_DIR}/zlib")

# Vorbis
set(ENGINE_DEP_VORBIS_DIR "${ENGINE_DEP_TOPLEVEL_DIR}/vorbis")
set(ENGINE_DEP_VORBIS_INCLUDE_DIR "${ENGINE_DEP_VORBIS_DIR}/include")
set(ENGINE_DEP_VORBIS_LIB_DIR "${ENGINE_DEP_VORBIS_DIR}/lib")

# Theora
set(ENGINE_DEP_THEORA_DIR "${ENGINE_DEP_TOPLEVEL_DIR}/theora")
set(ENGINE_DEP_THEORA_INCLUDE_DIR "${ENGINE_DEP_THEORA_DIR}/include")
set(ENGINE_DEP_THEORA_DEC_DIR "${ENGINE_DEP_THEORA_DIR}/lib/dec")
set(ENGINE_DEP_THEORA_X86_DIR "${ENGINE_DEP_THEORA_DEC_DIR}/x86")

# Speex
set(ENGINE_DEP_SPEEX_DIR "${ENGINE_DEP_TOPLEVEL_DIR}/speex")
set(ENGINE_DEP_SPEEX_INCLUDE_DIR "${ENGINE_DEP_SPEEX_DIR}/include")

# Ogg
set(ENGINE_DEP_OGG_DIR "${ENGINE_DEP_TOPLEVEL_DIR}/ogg")
set(ENGINE_DEP_OGG_INCLUDE_DIR "${ENGINE_DEP_OGG_DIR}/include")
set(ENGINE_DEP_OGG_SRC_DIR "${ENGINE_DEP_OGG_DIR}/src")

# Various includes
set(ENGINE_INCLUDE_TOPLEVEL_DIR "include")
set(ENGINE_INCLUDE_ENGINE_DIR "${ENGINE_INCLUDE_TOPLEVEL_DIR}/hat/engine")
set(ENGINE_INCLUDE_JAVA_DIR "${ENGINE_INCLUDE_TOPLEVEL_DIR}/hat/java")
set(ENGINE_INCLUDE_RESOURCE_DIR "${ENGINE_INCLUDE_TOPLEVEL_DIR}/hat/resource")
set(ENGINE_SRC_DIR "src")

# SDL is configuration-specific and Curl is a pain in the ass
# So, we have prebuilt binaries/libraries for Windows
if(WIN32)
	set(ENGINE_DEP_SDL_DIR "${ENGINE_DEP_TOPLEVEL_DIR}/sdl")
	set(ENGINE_DEP_SDL_INCLUDE_DIR "${ENGINE_DEP_SDL_DIR}/include")
	set(ENGINE_DEP_SDL_LIB_DIR "${ENGINE_DEP_SDL_DIR}/lib")

	set(ENGINE_DEP_CURL_DIR "${ENGINE_DEP_TOPLEVEL_DIR}/curl")
	set(ENGINE_DEP_CURL_INCLUDE_DIR "${ENGINE_DEP_CURL_DIR}/include")
	set(ENGINE_DEP_CURL_LIB_DIR "${ENGINE_DEP_CURL_DIR}/lib")

	set(CMAKE_LIBRARY_PATH 
		${CMAKE_LIBRARY_PATH} 
		${ENGINE_DEP_SDL_LIB_DIR}
		${ENGINE_DEP_CURL_LIB_DIR}
	)

	# External includes
	set(CMAKE_INCLUDE_PATH
		${CMAKE_INCLUDE_PATH}
		${ENGINE_DEP_SDL_INCLUDE_DIR}
		${ENGINE_DEP_CURL_INCLUDE_DIR}
	)
endif()


# These packages are actually needed and I rather not put them into the build
# system. I recommended OpenAL to be openal-soft.
find_package(SDL REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenAL REQUIRED)
find_package(OpenGL REQUIRED)

# Source lists
set(ENGINE_SRC_SERVER_SHARED_LIST
	# Server shared (needed for MSVC)
	"${SERVER_INCLUDE_SERVER_DIR}/bg_public.h"
	"${SERVER_INCLUDE_SERVER_DIR}/g_public.h"
)

set(ENGINE_SRC_CLIENT_SHARED_LIST
	# Client shared (needed for MSVC)
	"${CLIENT_INCLUDE_CLIENT_DIR}/cg_public.h"
)

set(ENGINE_SRC_GUI_SHARED_LIST
	# Gui shared (needed for MSVC)
	"${GUI_INCLUDE_GUI_DIR}/ui_public.h"
)

set(ENGINE_SRC_HEADER_LIST
	# Engine (needed for MSVC)
	"${ENGINE_INCLUDE_ENGINE_DIR}/client.h"
	"${ENGINE_INCLUDE_ENGINE_DIR}/cl_curl.h"
	"${ENGINE_INCLUDE_ENGINE_DIR}/cm_local.h"
	"${ENGINE_INCLUDE_ENGINE_DIR}/cm_polylib.h"
	"${ENGINE_INCLUDE_ENGINE_DIR}/cm_public.h"
	"${ENGINE_INCLUDE_ENGINE_DIR}/engine_tests.h"
	"${ENGINE_INCLUDE_ENGINE_DIR}/keycodes.h"
	"${ENGINE_INCLUDE_ENGINE_DIR}/keys.h"
	"${ENGINE_INCLUDE_ENGINE_DIR}/qal.h"
	"${ENGINE_INCLUDE_ENGINE_DIR}/qcommon.h"
	"${ENGINE_INCLUDE_ENGINE_DIR}/qfiles.h"
	"${ENGINE_INCLUDE_ENGINE_DIR}/q_platform.h"
	"${ENGINE_INCLUDE_ENGINE_DIR}/q_shared.h"
	"${ENGINE_INCLUDE_ENGINE_DIR}/server.h"
	"${ENGINE_INCLUDE_ENGINE_DIR}/snd_codec.h"
	"${ENGINE_INCLUDE_ENGINE_DIR}/snd_local.h"
	"${ENGINE_INCLUDE_ENGINE_DIR}/snd_public.h"
	"${ENGINE_INCLUDE_ENGINE_DIR}/surfaceflags.h"
	"${ENGINE_INCLUDE_ENGINE_DIR}/sys_loadlib.h"
	"${ENGINE_INCLUDE_ENGINE_DIR}/sys_local.h"
	"${ENGINE_INCLUDE_ENGINE_DIR}/unzip.h"
	"${ENGINE_INCLUDE_ENGINE_DIR}/vm_java.h"
)

set(ENGINE_SRC_JAVA_LIST
	# Java
	"${ENGINE_INCLUDE_JAVA_DIR}/xreal_client_Client.h"
	"${ENGINE_INCLUDE_JAVA_DIR}/xreal_client_renderer_Renderer.h"
	"${ENGINE_INCLUDE_JAVA_DIR}/xreal_server_game_GameEntity.h"
	"${ENGINE_INCLUDE_JAVA_DIR}/xreal_server_game_Player.h"
	"${ENGINE_INCLUDE_JAVA_DIR}/xreal_server_Server.h"
	"${ENGINE_INCLUDE_JAVA_DIR}/xreal_CVar.h"
	"${ENGINE_INCLUDE_JAVA_DIR}/xreal_Engine.h"
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

set(ENGINE_SRC_VORBIS_LIST
	# Vorbis source files
	"${ENGINE_DEP_VORBIS_LIB_DIR}/analysis.c"
	"${ENGINE_DEP_VORBIS_LIB_DIR}/bitrate.c"
	"${ENGINE_DEP_VORBIS_LIB_DIR}/block.c"
	"${ENGINE_DEP_VORBIS_LIB_DIR}/codebook.c"
	"${ENGINE_DEP_VORBIS_LIB_DIR}/envelope.c"
	"${ENGINE_DEP_VORBIS_LIB_DIR}/floor0.c"
	"${ENGINE_DEP_VORBIS_LIB_DIR}/floor1.c"
	"${ENGINE_DEP_VORBIS_LIB_DIR}/info.c"
	"${ENGINE_DEP_VORBIS_LIB_DIR}/lookup.c"
	"${ENGINE_DEP_VORBIS_LIB_DIR}/lpc.c"
	"${ENGINE_DEP_VORBIS_LIB_DIR}/lsp.c"
	"${ENGINE_DEP_VORBIS_LIB_DIR}/mapping0.c"
	"${ENGINE_DEP_VORBIS_LIB_DIR}/mdct.c"
	"${ENGINE_DEP_VORBIS_LIB_DIR}/psy.c"
	"${ENGINE_DEP_VORBIS_LIB_DIR}/registry.c"
	"${ENGINE_DEP_VORBIS_LIB_DIR}/res0.c"
	"${ENGINE_DEP_VORBIS_LIB_DIR}/sharedbook.c"
	"${ENGINE_DEP_VORBIS_LIB_DIR}/smallft.c"
	"${ENGINE_DEP_VORBIS_LIB_DIR}/synthesis.c"
	"${ENGINE_DEP_VORBIS_LIB_DIR}/vorbisfile.c"
	"${ENGINE_DEP_VORBIS_LIB_DIR}/window.c"
)

set(ENGINE_SRC_THEORA_LIST
	"${ENGINE_DEP_THEORA_DEC_DIR}/apiwrapper.c"
	"${ENGINE_DEP_THEORA_DEC_DIR}/bitpack.c"
	"${ENGINE_DEP_THEORA_DEC_DIR}/decapiwrapper.c"
	"${ENGINE_DEP_THEORA_DEC_DIR}/decinfo.c"
	"${ENGINE_DEP_THEORA_DEC_DIR}/decode.c"
	"${ENGINE_DEP_THEORA_DEC_DIR}/dequant.c"
	"${ENGINE_DEP_THEORA_DEC_DIR}/fragment.c"
	"${ENGINE_DEP_THEORA_DEC_DIR}/huffdec.c"
	"${ENGINE_DEP_THEORA_DEC_DIR}/idct.c"
	"${ENGINE_DEP_THEORA_DEC_DIR}/internal.c"
	"${ENGINE_DEP_THEORA_DEC_DIR}/quant.c"
	"${ENGINE_DEP_THEORA_DEC_DIR}/state.c"
	"${ENGINE_DEP_THEORA_DEC_DIR}/thinfo.c"
	"${ENGINE_DEP_THEORA_X86_DIR}/mmxfrag.c"
	"${ENGINE_DEP_THEORA_X86_DIR}/mmxidct.c"
	"${ENGINE_DEP_THEORA_X86_DIR}/mmxstate.c"
	"${ENGINE_DEP_THEORA_X86_DIR}/x86state.c"
)

set(ENGINE_SRC_SPEEX_LIST
	"${ENGINE_DEP_SPEEX_DIR}/bits.c"
	"${ENGINE_DEP_SPEEX_DIR}/buffer.c"
	"${ENGINE_DEP_SPEEX_DIR}/cb_search.c"
	"${ENGINE_DEP_SPEEX_DIR}/exc_10_16_table.c"
	"${ENGINE_DEP_SPEEX_DIR}/exc_10_32_table.c"
	"${ENGINE_DEP_SPEEX_DIR}/exc_20_32_table.c"
	"${ENGINE_DEP_SPEEX_DIR}/exc_5_256_table.c"
	"${ENGINE_DEP_SPEEX_DIR}/exc_5_64_table.c"
	"${ENGINE_DEP_SPEEX_DIR}/exc_8_128_table.c"
	"${ENGINE_DEP_SPEEX_DIR}/fftwrap.c"
	"${ENGINE_DEP_SPEEX_DIR}/filterbank.c"
	"${ENGINE_DEP_SPEEX_DIR}/filters.c"
	"${ENGINE_DEP_SPEEX_DIR}/gain_table.c"
	"${ENGINE_DEP_SPEEX_DIR}/gain_table_lbr.c"
	"${ENGINE_DEP_SPEEX_DIR}/hexc_10_32_table.c"
	"${ENGINE_DEP_SPEEX_DIR}/hexc_table.c"
	"${ENGINE_DEP_SPEEX_DIR}/high_lsp_tables.c"
	"${ENGINE_DEP_SPEEX_DIR}/jitter.c"
	"${ENGINE_DEP_SPEEX_DIR}/kiss_fft.c"
	"${ENGINE_DEP_SPEEX_DIR}/kiss_fftr.c"
	"${ENGINE_DEP_SPEEX_DIR}/lsp_tables_nb.c"
	"${ENGINE_DEP_SPEEX_DIR}/ltp.c"
	"${ENGINE_DEP_SPEEX_DIR}/mdf.c"
	"${ENGINE_DEP_SPEEX_DIR}/modes.c"
	"${ENGINE_DEP_SPEEX_DIR}/modes_wb.c"
	"${ENGINE_DEP_SPEEX_DIR}/nb_celp.c"
	"${ENGINE_DEP_SPEEX_DIR}/preprocess.c"
	"${ENGINE_DEP_SPEEX_DIR}/quant_lsp.c"
	"${ENGINE_DEP_SPEEX_DIR}/resample.c"
	"${ENGINE_DEP_SPEEX_DIR}/sb_celp.c"
	"${ENGINE_DEP_SPEEX_DIR}/smallft.c"
	"${ENGINE_DEP_SPEEX_DIR}/speex.c"
	"${ENGINE_DEP_SPEEX_DIR}/speex_callbacks.c"
	"${ENGINE_DEP_SPEEX_DIR}/speex_header.c"
	"${ENGINE_DEP_SPEEX_DIR}/speex_lpc.c"
	"${ENGINE_DEP_SPEEX_DIR}/speex_lsp.c"
	"${ENGINE_DEP_SPEEX_DIR}/speex_window.c"
	"${ENGINE_DEP_SPEEX_DIR}/stereo.c"
	"${ENGINE_DEP_SPEEX_DIR}/vbr.c"
	"${ENGINE_DEP_SPEEX_DIR}/vq.c"
)

set(ENGINE_SRC_OGG_LIST
	"${ENGINE_DEP_OGG_SRC_DIR}/bitwise.c"
	"${ENGINE_DEP_OGG_SRC_DIR}/framing.c"
)

set(ENGINE_SRC_LIST
	# Hat source files
	"${ENGINE_SRC_DIR}/cin_ogm.c"
	"${ENGINE_SRC_DIR}/cl_avi.c"
	"${ENGINE_SRC_DIR}/cl_cgame.c"
	"${ENGINE_SRC_DIR}/cl_cgame_java.c"
	"${ENGINE_SRC_DIR}/cl_cin.c"
	"${ENGINE_SRC_DIR}/cl_console.c"
	"${ENGINE_SRC_DIR}/cl_curl.c"
	"${ENGINE_SRC_DIR}/cl_input.c"
	"${ENGINE_SRC_DIR}/cl_keys.c"
	"${ENGINE_SRC_DIR}/cl_main.c"
	"${ENGINE_SRC_DIR}/cl_net_chan.c"
	"${ENGINE_SRC_DIR}/cl_parse.c"
	"${ENGINE_SRC_DIR}/cl_scrn.c"
	"${ENGINE_SRC_DIR}/cl_ui.c"
	"${ENGINE_SRC_DIR}/cl_ui_java.c"
	"${ENGINE_SRC_DIR}/cmd.c"
	"${ENGINE_SRC_DIR}/cm_load.c"
	"${ENGINE_SRC_DIR}/cm_patch.c"
	"${ENGINE_SRC_DIR}/cm_polylib.c"
	"${ENGINE_SRC_DIR}/cm_test.c"
	"${ENGINE_SRC_DIR}/cm_trace.c"
	"${ENGINE_SRC_DIR}/cm_trisoup.c"
	"${ENGINE_SRC_DIR}/common.c"
	"${ENGINE_SRC_DIR}/con_log.c"
	"${ENGINE_SRC_DIR}/cvar.c"
	"${ENGINE_SRC_DIR}/files.c"
	"${ENGINE_SRC_DIR}/engine_tests.cpp"
	"${ENGINE_SRC_DIR}/huffman.c"
	"${ENGINE_SRC_DIR}/md4.c"
	"${ENGINE_SRC_DIR}/md5.c"
	"${ENGINE_SRC_DIR}/msg.c"
	"${ENGINE_SRC_DIR}/net_chan.c"
	"${ENGINE_SRC_DIR}/net_ip.c"
	"${ENGINE_SRC_DIR}/parse.c"
	"${ENGINE_SRC_DIR}/qal.c"
	"${ENGINE_SRC_DIR}/q_math.c"
	"${ENGINE_SRC_DIR}/q_shared.c"
	"${ENGINE_SRC_DIR}/sdl_input.c"
	"${ENGINE_SRC_DIR}/sdl_snd.c"
	"${ENGINE_SRC_DIR}/snd_codec.c"
	"${ENGINE_SRC_DIR}/snd_codec_ogg.c"
	"${ENGINE_SRC_DIR}/snd_codec_wav.c"
	"${ENGINE_SRC_DIR}/snd_dma.c"
	"${ENGINE_SRC_DIR}/snd_main.c"
	"${ENGINE_SRC_DIR}/snd_mem.c"
	"${ENGINE_SRC_DIR}/snd_mix.c"
	"${ENGINE_SRC_DIR}/snd_openal.c"
	"${ENGINE_SRC_DIR}/sv_bot.c"
	"${ENGINE_SRC_DIR}/sv_ccmds.c"
	"${ENGINE_SRC_DIR}/sv_client.c"
	"${ENGINE_SRC_DIR}/sv_game.c"
	"${ENGINE_SRC_DIR}/sv_game_java.c"
	"${ENGINE_SRC_DIR}/sv_init.c"
	"${ENGINE_SRC_DIR}/sv_main.c"
	"${ENGINE_SRC_DIR}/sv_net_chan.c"
	"${ENGINE_SRC_DIR}/sv_snapshot.c"
	"${ENGINE_SRC_DIR}/sv_world.c"
	"${ENGINE_SRC_DIR}/sys_main.c"
	"${ENGINE_SRC_DIR}/v8/type_conversion.cpp"
	"${ENGINE_SRC_DIR}/unzip.c"
	"${ENGINE_SRC_DIR}/vm.c"
	"${ENGINE_SRC_DIR}/vm_java.c"
)

set(ENGINE_JS_WEAPON
	"${ENGINE_SRC_DIR}/javascript/weapon/class.cpp"
	"${ENGINE_SRC_DIR}/javascript/weapon/manager.cpp"
	"${ENGINE_SRC_DIR}/javascript/weapon/tools.cpp"
	"${ENGINE_INCLUDE_ENGINE_DIR}/javascript/weapon/class.hpp"
	"${ENGINE_INCLUDE_ENGINE_DIR}/javascript/weapon/manager.hpp"
	"${ENGINE_INCLUDE_ENGINE_DIR}/javascript/weapon.hpp"
)


# OS-specific code
if(UNIX)
	message(STATUS "(sys_unix.c) Unix-like operating system")
	set(UNIX_SRC_LIST
		"${ENGINE_SRC_DIR}/sys_unix.c"
		"${ENGINE_SRC_DIR}/con_passive.c"
	)
	set(ENGINE_SRC_LIST ${ENGINE_SRC_LIST} ${UNIX_SRC_LIST})
	message(STATUS "Unix-like operating system.")
	message(STATUS "Additional includes: ${UNIX_SRC_LIST}")
elseif(APPLE)
	set(APPLE_SRC_LIST
		"${ENGINE_SRC_DIR}/sys_unix.c"
	)
	set(ENGINE_SRC_LIST ${ENGINE_SRC_LIST} ${APPLE_SRC_LIST})
	message(STATUS "(${APPLE_SRC_LIST}) Apple-like operating system")
	message(STATUS "Apple-like operating system.")
	message(STATUS "Additional includes: ${APPLE_SRC_LIST}")
elseif(WIN32)
	set(WIN32_LIB_LIST
		"winmm.lib"
		"ws2_32.lib"
		"PSAPI.lib"
		"user32.lib"
	)

	set(WIN32_RESOURCE_LIST
		"${ENGINE_INCLUDE_RESOURCE_DIR}/win_resource.h"
		"${ENGINE_INCLUDE_RESOURCE_DIR}/win_resource.rc"
		"${ENGINE_INCLUDE_RESOURCE_DIR}/rr.ico"	
	)

	set(WIN32_SRC_LIST
		"${ENGINE_SRC_DIR}/con_win32.c"
		"${ENGINE_SRC_DIR}/sys_win32.c"
	)

	include_directories(
		"${ENGINE_DEP_SDL_INCLUDE_DIR}"
		"${ENGINE_DEP_CURL_INCLUDE_DIR}"
	)


	if(${HAT_EXPAND_PLATFORM})
		source_group("(win32-specific) Sources" FILES ${WIN32_SRC_LIST})
		source_group("(win32-specific) Resources" FILES ${WIN32_RESOURCE_LIST})
	else()
		source_group("Resource Files" FILES ${WIN32_RESOURCE_LIST})
	endif()

	message(STATUS "Windows-like operating system.")
	message(STATUS "Additional includes: ${WIN32_SRC_LIST} ${WIN32_RESOURCE_LIST}")
	message(STATUS "Additional libraries: ${WIN32_LIB_LIST}")
else()
	message(FATAL_ERROR "Unsupported operating system.")
endif()


# Define the final source list
set(ENGINE_SRC_ALL_LIST
	${ENGINE_SRC_SERVER_SHARED_LIST}
	${ENGINE_SRC_CLIENT_SHARED_LIST}
	${ENGINE_SRC_GUI_SHARED_LIST}
	${ENGINE_SRC_HEADER_LIST}
	${ENGINE_SRC_ZLIB_LIST}
	${ENGINE_SRC_JAVA_LIST}
	${ENGINE_SRC_VORBIS_LIST}
	${ENGINE_SRC_THEORA_LIST}
	${ENGINE_SRC_SPEEX_LIST}
	${ENGINE_SRC_OGG_LIST}
	${ENGINE_SRC_LIST}
	${WIN32_SRC_LIST}
	${WIN32_RESOURCE_LIST}
	${APPLE_SRC_LIST}
	${UNIX_SRC_LIST}
	${ENGINE_JS_WEAPON}
)



# Project include
include_directories(
	"${ENGINE_INCLUDE_TOPLEVEL_DIR}"
	"${SERVER_INCLUDE_TOPLEVEL_DIR}"
	"${CLIENT_INCLUDE_TOPLEVEL_DIR}"
	"${GUI_INCLUDE_TOPLEVEL_DIR}"
	"${RENDERER_INCLUDE_TOPLEVEL_DIR}"
	"${ENGINE_DEP_ZLIB_DIR}"
	"${ENGINE_DEP_VORBIS_INCLUDE_DIR}"
	"${ENGINE_DEP_THEORA_INCLUDE_DIR}"
	"${ENGINE_DEP_SPEEX_INCLUDE_DIR}"
	"${ENGINE_DEP_OGG_INCLUDE_DIR}"
	"${SDL_INCLUDE_DIR}"
	"${CURL_INCLUDE_DIR}"
	"${OPENAL_INCLUDE_DIR}"
	"${V8_INCLUDE_DIR}"
)

# Group the source code
source_group("Header Files" FILES ${ENGINE_SRC_HEADER_LIST})
source_group("Source Files" FILES ${ENGINE_SRC_SERVER_SHARED_LIST} ${ENGINE_SRC_CLIENT_SHARED_LIST} ${ENGINE_SRC_GUI_SHARED_LIST} ${ENGINE_SRC_LIST})

source_group("JavaScript\\Weapon" FILES ${ENGINE_JS_WEAPON})

if(${HAT_EXPAND_DEPENDENCIES})
	source_group("(library) Java" FILES ${ENGINE_SRC_JAVA_LIST})
	source_group("(library) Vorbis" FILES ${ENGINE_SRC_VORBIS_LIST})
	source_group("(library) Theora" FILES ${ENGINE_SRC_THEORA_LIST})
	source_group("(library) Speex" FILES ${ENGINE_SRC_SPEEX_LIST})
	source_group("(library) Ogg" FILES ${ENGINE_SRC_OGG_LIST})
	source_group("(library) Zlib" FILES ${ENGINE_SRC_ZLIB_LIST})
else()
	source_group("External Libraries"
		FILES
		${ENGINE_SRC_JAVA_LIST}
		${ENGINE_SRC_OGG_LIST}
		${ENGINE_SRC_VORBIS_LIST}
		${ENGINE_SRC_THEORA_LIST}
		${ENGINE_SRC_SPEEX_LIST}
		${ENGINE_SRC_ZLIB_LIST}
	)
endif()

if(WIN32)
	set(OS_SPECIFIC_LIBRARIES "winmm.lib" "ws2_32.lib")
endif()

# Setup the executable
add_executable(engine ${ENGINE_SRC_ALL_LIST})
target_link_libraries(engine ${OS_SPECIFIC_LIBRARIES} ${V8_LIBRARY} ${WIN32_LIB_LIST} ${SDL_LIBRARY} ${OPENAL_LIBRARY} ${CURL_LIBRARY} ${OPENGL_LIBRARY})
add_definitions(
	-DUSE_CURL
	-DUSE_OPENAL
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

if (WIN32)
	set_target_properties(engine PROPERTIES LINK_FLAGS
		"/FIXED:NO /DYNAMICBASE:NO"
	)
endif(WIN32)
