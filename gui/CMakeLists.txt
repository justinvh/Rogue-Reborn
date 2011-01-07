find_package(V8 REQUIRED)

if(V8_FOUND)
	message(STATUS "- V8 Found............ ${V8_INCLUDE_DIR}")
	include_directories(${V8_INCLUDE_DIR})
endif(V8_FOUND)


# Miscellaneous project includes
set(MISC_INCLUDES
	${ENGINE_INCLUDE_DIR}/hat/v8/easy.hpp
)

# engine/include/hat/engine/
set(ENGINE_INCLUDES 
	q_shared.h
	keycodes.h
)

# gui/include/hat/gui/
set(GUI_INCLUDES
	menudef.h
	ui_local.h
	ui_public.h
	element.hpp
	eventful.hpp
	gui.hpp
	image.hpp
)

# server/src
set(SERVER_SRC
	bg_misc.c
)

# engine/src
set(ENGINE_SRC
	q_math.c
	q_shared.c
)

# gui/src
set(GUI_SRC
	gui.def
	ui_main.cpp
	ui_syscalls.c
	element.cpp
	eventful.cpp
	gui.cpp
	image.cpp
	atoms.cpp
)

# Parse file sources
set(GROUPS ENGINE GUI SERVER CLIENT PHYSICS)
set(SRC_ALL ${MISC_INCLUDES} ${MISC_SRC})
foreach(GROUP ${GROUPS})
	foreach(FILE_SRC ${${GROUP}_INCLUDES})
		set(SRC_ALL ${SRC_ALL} ${${GROUP}_HAT_DIR}/${FILE_SRC})
	endforeach(FILE_SRC)
	foreach(FILE_SRC ${${GROUP}_SRC})
		set(SRC_ALL ${SRC_ALL} ${${GROUP}_SRC_DIR}/${FILE_SRC})
	endforeach(FILE_SRC)
	include_directories(${${GROUP}_INCLUDE_DIR})
endforeach(GROUP)

# Additional includes


if(WIN32)
	set(OS_SPECIFIC_LIBRARIES "winmm.lib" "ws2_32.lib")
endif()

# Group the source code
source_group("Header Files" FILES ${GUI_SRC_HEADER_LIST} ${GUI_SRC_ENGINE_SHARED_LIST})
source_group("Source Files" FILES ${GUI_SRC_LIST})

# Setup the library 
add_library(gui SHARED ${GUI_SRC_ALL_LIST})
target_link_libraries(gui ${V8_LIBRARY} ${OS_SPECIFIC_LIBRARIES})
set_target_properties(gui PROPERTIES PREFIX "")

if(WIN32)
	set_target_properties(gui PROPERTIES LINK_FLAGS
		"/BASE:0x40000000 /DYNAMICBASE:NO"
	)
endif()

install(TARGETS gui DESTINATION ${CMAKE_INSTALL_PREFIX}/base/)
