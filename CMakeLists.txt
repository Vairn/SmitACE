cmake_minimum_required(VERSION 3.14.0)

option(ACE_HOST "Build with ACE host SDL chipset backend (Windows/Linux)" OFF)

if(ACE_HOST)
	project(Smite LANGUAGES C)
else()
	project(Smite LANGUAGES ASM C)
endif()

# Force 68020 CPU target for Amiga builds
if(NOT ACE_HOST)
	set(M68K_CPU "68020" CACHE STRING "Target CPU model" FORCE)
endif()

#set(CMAKE_C_STANDARD 17)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.c src/game/*.c src/misc/*.c src/maze/*.c src/gfx/*.c include/*.h)
if(ACE_HOST)
	# 68020-only stack-pointer test; not part of the game and will not assemble on host.
	list(FILTER SOURCES EXCLUDE REGEX "tail_call_test\\.c$")
endif()
set(ACE_USE_AGA_FEATURES ON CACHE BOOL "Enable ACE AGA feature set" FORCE)
set(ACE_USE_ECS_FEATURES ON CACHE BOOL "Enable ACE ECS feature set" FORCE)
set(ACE_DEBUG ON)
set(ACE_DEBUG_UAE ON)
if(ACE_HOST)
	set(CMAKE_C_STANDARD 11)
	if(WIN32 AND EXISTS "C:/msys64/ucrt64")
		list(APPEND CMAKE_PREFIX_PATH "C:/msys64/ucrt64")
	endif()
endif()

if(ELF2HUNK AND NOT ACE_HOST)
	set(SMITE_EXECUTABLE smite.elf)
	set(SMITE_OUTPUT_EXECUTABLE smite.exe)
	add_executable(${SMITE_EXECUTABLE} ${SOURCES})
	
	target_link_libraries(${SMITE_EXECUTABLE} -Wl,-Map=smite.map)
	
	add_custom_command(
		TARGET ${SMITE_EXECUTABLE} POST_BUILD
		COMMAND ${ELF2HUNK} ${SMITE_EXECUTABLE} ${SMITE_OUTPUT_EXECUTABLE}
	)
	add_custom_command(
		TARGET ${SMITE_EXECUTABLE} POST_BUILD
		COMMAND ${OBJDUMP} --disassemble -m68020 -S ${SMITE_EXECUTABLE} > smite.s
	)
else()
	SET(SMITE_EXECUTABLE smite)
	SET(SMITE_OUTPUT_EXECUTABLE smite)
	
	add_executable(${SMITE_EXECUTABLE} ${SOURCES})
endif()



#if(AMIGA)
add_subdirectory(deps/ace ace)
#endif()


# Version stuff
string(TIMESTAMP YEAR "%y")
string(TIMESTAMP DAY "%d")
string(TIMESTAMP MONTH "%m")
MATH(EXPR VER_MAJOR "0 + ${YEAR}")
MATH(EXPR VER_MINOR "0 + ${MONTH}")
MATH(EXPR VER_FIX "0 + ${DAY}")
set(VERSION "${VER_MAJOR}.${VER_MINOR}.${VER_FIX}")

target_compile_definitions(${SMITE_EXECUTABLE} PRIVATE smite_VERSION="${VERSION}")
include_directories(${SDL2_INCLUDE_DIRS} include/)
target_link_libraries(${SMITE_EXECUTABLE} ace)

if(ACE_HOST)
	if(NOT MSVC)
		target_compile_options(${SMITE_EXECUTABLE} PRIVATE -Wall -Wextra -g -fno-omit-frame-pointer)
	endif()
	if(TARGET SDL2::SDL2main)
		target_link_libraries(${SMITE_EXECUTABLE} SDL2::SDL2main)
	endif()
else()
	target_compile_options(${SMITE_EXECUTABLE} PUBLIC -fomit-frame-pointer)
endif()

# m68k-amiga-elf + LTO: GCC warns on many __attribute__s ACE uses ((hot))/((cold))/etc.
# as "visibility attribute not supported in this configuration"; harmless noise.
if(AMIGA AND NOT ACE_HOST)
	target_compile_options(ace PRIVATE -Wno-attributes)
	target_compile_options(${SMITE_EXECUTABLE} PRIVATE -Wno-attributes)
endif()

# CRITICAL: Disable aggressive optimizations for 68020+ - known to cause crashes
# -fno-optimize-sibling-calls: Prevents tail-call optimization that generates invalid branches
# Per-function attributes don't work for function pointer calls, so we need global disable
#target_compile_options(${SMITE_EXECUTABLE} PRIVATE  -fno-optimize-sibling-calls)
#target_compile_options(ace PRIVATE   -fno-optimize-sibling-calls)

if(GAME_DEBUG)
	target_compile_definitions(${SMITE_EXECUTABLE} PRIVATE GAME_DEBUG)
	target_compile_definitions(ace PUBLIC ACE_DEBUG_ALL ACE_DEBUG_UAE)
endif()



set(RES_DIR ${CMAKE_CURRENT_LIST_DIR}/res)
set(DATA_DIR ${CMAKE_CURRENT_BINARY_DIR}/data)
set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

add_custom_command(
    TARGET ${SMITE_EXECUTABLE}
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DATA_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/data" "${DATA_DIR}"
    COMMAND ${CMAKE_COMMAND}
    -DLANGUAGE:STRING="C"
    -DHEADER_DIR:PATH="${CMAKE_SOURCE_DIR}/src"
    -DCACHE_DIR:PATH="${CMAKE_SOURCE_DIR}"
    -DCACHE_FILENAME:="BUILD_NUM"
    -DHEADER_GUARD:STRING="__BUILD_NUMBER_H__INCLUDED__"
    -DVERSION_MAJOR="${PROJECT_VERSION_MAJOR}"
    -DVERSION_MINOR="${PROJECT_VERSION_MINOR}"
    -P "${CMAKE_SOURCE_DIR}/build_number.cmake"
)

# Generating ZIP
set(smite_ZIP_NAME "${CMAKE_PROJECT_NAME} ${VER_MAJOR}_${VER_MINOR}_${VER_FIX}")
add_custom_target(generateZip COMMAND
	${CMAKE_COMMAND} -E tar "cf" "${smite_ZIP_NAME}.zip" --format=zip
	"${CMAKE_CURRENT_BINARY_DIR}/${SMITE_OUTPUT_EXECUTABLE}" "${DATA_DIR}"
	COMMENT "Generating ${smite_ZIP_NAME}.zip"
)

if(NOT ACE_HOST)
	# Generating ADF
	set(ADF_DIR "${CMAKE_CURRENT_BINARY_DIR}/adf")
	add_custom_target(generateAdf
		COMMAND ${CMAKE_COMMAND} -E make_directory "${ADF_DIR}/s"
		COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/${SMITE_OUTPUT_EXECUTABLE}" "${ADF_DIR}"
		COMMAND ${CMAKE_COMMAND} -E copy_directory "${DATA_DIR}" "${ADF_DIR}/data"
		COMMAND ${CMAKE_COMMAND} -E echo "${SMITE_OUTPUT_EXECUTABLE}" > "${ADF_DIR}/s/startup-sequence"
		COMMAND exe2adf -l ${CMAKE_PROJECT_NAME} -a "${CMAKE_PROJECT_NAME}.adf" -d ${ADF_DIR}
		COMMAND ${CMAKE_COMMAND} -E rm -rf "${ADF_DIR}"
		COMMENT "Generating ADF file"
	)
endif()

if(ACE_HOST)
	# Place SDL2.dll next to smite.exe so the Native build runs from the build dir.
	set(_SDL2_DLL "")
	if(TARGET SDL2::SDL2)
		get_target_property(_SDL2_DLL SDL2::SDL2 IMPORTED_LOCATION)
		if(NOT _SDL2_DLL)
			get_target_property(_SDL2_DLL SDL2::SDL2 IMPORTED_LOCATION_RELEASE)
		endif()
		if(_SDL2_DLL MATCHES "\\.dll\\.a$")
			get_filename_component(_sdl_libdir "${_SDL2_DLL}" DIRECTORY)
			get_filename_component(_sdl_root "${_sdl_libdir}" DIRECTORY)
			set(_SDL2_DLL "${_sdl_root}/bin/SDL2.dll")
		endif()
	endif()
	if(NOT _SDL2_DLL OR NOT EXISTS "${_SDL2_DLL}")
		find_file(_SDL2_DLL NAMES SDL2.dll PATHS ${CMAKE_PREFIX_PATH} ENV PATH PATH_SUFFIXES bin NO_CACHE)
	endif()
	if(NOT _SDL2_DLL OR NOT EXISTS "${_SDL2_DLL}")
		foreach(_sdl_guess
			"${CMAKE_PREFIX_PATH}/bin/SDL2.dll"
			"C:/msys64/ucrt64/bin/SDL2.dll"
		)
			if(EXISTS "${_sdl_guess}")
				set(_SDL2_DLL "${_sdl_guess}")
				break()
			endif()
		endforeach()
	endif()
	if(_SDL2_DLL AND EXISTS "${_SDL2_DLL}")
		add_custom_command(
			TARGET ${SMITE_EXECUTABLE} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_SDL2_DLL}" "$<TARGET_FILE_DIR:${SMITE_EXECUTABLE}>"
			COMMENT "Copy SDL2.dll next to smite"
		)
	endif()
endif()
