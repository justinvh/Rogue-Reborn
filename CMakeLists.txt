# Copyright (c) 2011, Justin Bruce Van Horne <justinvh@gmail.com>

cmake_minimum_required(VERSION 2.8)
project(ROGUE_REBORN)

# Establish some basic paths
set(ROGUE_REBORN_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/engine/modules)
set(ENGINE_DEP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/engine/dep)
set(RENDERER_DEP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/renderer/dep)

#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!#
#! Add projects to this variable !#
#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!#
set(AVAILABLE_PROJECTS GUI CLIENT SERVER RENDERER PHYSICS ENGINE)

# GENERATE_HAT_SOURCES 
# generate_hat_sources() will consume the environment looking for
# <PROJECT>_INCLUDES and <PROJECT>_SRC lists and then add them to
# the include path as well as populate a variable called SRC_ALL
# which is then used to add_executable, add_shared_library, etc.
#
# To use this function you just define PROJECT_INCLUDES and PROJECT_SRC
# within the CMakeLists.txt for that particular project.
#
# Example:
# 	set(ENGINE_SRC q_shared.c q_platform.c)
#	set(ENGINE_INCLUDES q_shared.h q_platform.h)
#	set(CLIENT_SRC client.cpp client_file.c)
#	set(CLIENT_INCLUDES client.h)
#	generate_hat_sources()
macro(generate_hat_sources)
set(PROJECTS ${AVAILABLE_PROJECTS} ${ARGV})
foreach(GROUP ${PROJECTS})
	foreach(FILE_SRC ${${GROUP}_INCLUDES})
		if(DEFINED ${GROUP}_HAT_DIR)
			set(SRC_ALL ${SRC_ALL} ${${GROUP}_HAT_DIR}/${FILE_SRC})
			source_group("Header Files" 
				FILES ${${GROUP}_HAT_DIR}/${FILE_SRC})
		else()
			set(SRC_ALL ${SRC_ALL} ${FILE_SRC})
			source_group("Header Files" 
				FILES ${FILE_SRC})
		endif()
	endforeach(FILE_SRC)
	foreach(FILE_SRC ${${GROUP}_SRC})
		if(DEFINED ${GROUP}_SRC_DIR)
			set(SRC_ALL ${SRC_ALL} ${${GROUP}_SRC_DIR}/${FILE_SRC})
			source_group("Source Files" 
				FILES ${${GROUP}_SRC_DIR}/${FILE_SRC})
		else()
			set(SRC_ALL ${SRC_ALL} ${FILE_SRC})
			source_group("Source Files" 
				FILES ${FILE_SRC})
		endif()
	endforeach(FILE_SRC)
	include_directories(${${GROUP}_INCLUDE_DIR})
endforeach(GROUP)
endmacro(generate_hat_sources)

# GENERATE_EXTERNAL_SOURCES 
# Acts just like generate_hat_sources() except it takes a list of
# external sources.
macro(generate_external_sources)
foreach(GROUP ${ARGV})
	foreach(FILE_SRC ${${GROUP}_INCLUDES})
		set(SRC_ALL ${SRC_ALL} ${${GROUP}_INCLUDE_DIR}/${FILE_SRC})
		source_group("External Files\\${GROUP}\\Header Files" 
				FILES ${${GROUP}_INCLUDE_DIR}/${FILE_SRC})
	endforeach(FILE_SRC)
	foreach(FILE_SRC ${${GROUP}_SRC})
		set(SRC_ALL ${SRC_ALL} ${${GROUP}_SRC_DIR}/${FILE_SRC})
		source_group("External Files\\${GROUP}\\Source Files" 
				FILES ${${GROUP}_SRC_DIR}/${FILE_SRC})
	endforeach(FILE_SRC)
	include_directories(${${GROUP}_INCLUDE_DIR})
endforeach(GROUP)
endmacro(generate_external_sources)

# Setup each project
foreach(VAR ${AVAILABLE_PROJECTS})
	set(INCLUDE_VAR ${VAR}_INCLUDE_DIR)
	set(SRC_VAR ${VAR}_SRC_DIR)
	set(HAT_VAR ${VAR}_HAT_DIR)
	string(TOLOWER ${VAR} LVAR)
	set(${INCLUDE_VAR} ${ROGUE_REBORN_DIR}/${LVAR}/include)
	set(${HAT_VAR} ${${INCLUDE_VAR}}/hat/${LVAR})
	set(${SRC_VAR} ${ROGUE_REBORN_DIR}/${LVAR}/src)
endforeach(VAR)

# Output results
foreach(VAR ${AVAILABLE_PROJECTS})
	set(INCLUDE_VAR ${VAR}_INCLUDE_DIR)
	set(SRC_VAR ${VAR}_SRC_DIR)
	set(HAT_VAR ${VAR}_HAT_DIR)
	string(TOLOWER ${VAR} LVAR)
	message(STATUS "Generating ${VAR}...")
	message(STATUS "- include directory... ${${INCLUDE_VAR}}")
	message(STATUS "- hat directory....... ${${HAT_VAR}}")
	message(STATUS "- src directory....... ${${SRC_VAR}}")
	add_subdirectory(${LVAR})
endforeach(VAR)
