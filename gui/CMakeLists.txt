# Rogue Reborn GUI
find_package(V8 REQUIRED)

if(V8_FOUND)
	message(STATUS "- v8 include.......... ${V8_INCLUDE_DIR}")
	message(STATUS "- v8 library.......... ${V8_LIBRARY}")
	include_directories(${V8_INCLUDE_DIR})
endif(V8_FOUND)


# Operating specific libraries and flags
if(WIN32)
	set(SPECIFIC_LIBRARIES "winmm.lib" "ws2_32.lib")
	message(STATUS "- win32 libraries..... ${SPECIFIC_LIBRARIES}")
	if(NOT RR_DISABLE_IMAGE_BASE)
		set(SPECIFIC_LINKER_FLAGS "/BASE:0x40000000 /DYNAMICBASE:NO")
		message(STATUS "- win32 linker flags.. ${SPECIFIC_LINKER_FLAGS}")
	endif(NOT RR_DISABLE_IMAGE_BASE)
elseif(UNIX AND NOT RR_DISABLE_IMAGE_BASE)
	set(SPECIFIC_LINKER_FLAGS "--image-base=0x40000000")
	message(STATUS "- unix linker flags... ${SPECIFIC_LINKER_FLAGS}")
endif(WIN32)

set(LIBRARIES ${SPECIFIC_LIBRARIES} ${V8_LIBRARY})

# Miscellaneous project includes
set(ENGINE_INCLUDES v8/easy.hpp)

# Engine includes and sources
set(ENGINE_INCLUDES q_shared.h keycodes.h)
set(ENGINE_SRC q_math.c q_shared.c)

# Server sources
set(SERVER_SRC bg_misc.c)

# Gui includes and sources
set(GUI_INCLUDES
	menudef.h
	ui_local.h
	ui_public.h
	element.hpp
	eventful.hpp
	gui.hpp
	image.hpp
	gui.def
)

set(GUI_SRC
	ui_main.cpp
	ui_syscalls.c
	element.cpp
	eventful.cpp
	gui.cpp
	image.cpp
	atoms.cpp
)

generate_hat_sources()

# Setup the library 
add_library(gui SHARED ${SRC_ALL})
target_link_libraries(gui ${LIBRARIES})
set_target_properties(gui PROPERTIES PREFIX "")

if(DEFINED SPECIFIC_LINKER_FLAGS)
	set_target_properties(gui PROPERTIES 
		LINK_FLAGS ${SPECIFIC_LINKER_FLAGS})
endif(DEFINED SPECIFIC_LINKER_FLAGS)

# Installation
install(TARGETS gui DESTINATION ${CMAKE_INSTALL_PREFIX}/base/)
