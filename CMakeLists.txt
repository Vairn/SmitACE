cmake_minimum_required(VERSION 3.14.0)
project(Smite LANGUAGES ASM C)

set(CMAKE_C_STANDARD 11)

file(GLOB_RECURSE SOURCES src/*.c src/game/*.c src/misc/*.c include/*.h)



if(ELF2HUNK)
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
		COMMAND ${OBJDUMP} --disassemble -S ${SMITE_EXECUTABLE} > smite.s
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
target_link_libraries(${SMITE_EXECUTABLE} ace )  
target_compile_options(${SMITE_EXECUTABLE} PUBLIC -Wall)


if(GAME_DEBUG)
	target_compile_definitions(${SMITE_EXECUTABLE} PRIVATE GAME_DEBUG)
	target_compile_definitions(ace PUBLIC ACE_DEBUG_ALL ACE_DEBUG_UAE)
endif()


if(GAME_DEBUG)
	target_compile_definitions(${PAK_EXECUTABLE} PRIVATE GAME_DEBUG)
	target_compile_definitions(ace PUBLIC ACE_DEBUG_ALL ACE_DEBUG_UAE)
endif()


set(RES_DIR ${CMAKE_CURRENT_LIST_DIR}/res)
set(DATA_DIR ${CMAKE_CURRENT_BINARY_DIR}/data)
set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)


# Generating ZIP
set(smite_ZIP_NAME "${CMAKE_PROJECT_NAME} ${VER_MAJOR}_${VER_MINOR}_${VER_FIX}")
add_custom_target(generateZip COMMAND
	${CMAKE_COMMAND} -E tar "cf" "${smite_ZIP_NAME}.zip" --format=zip
	"${CMAKE_CURRENT_BINARY_DIR}/${SMITE_OUTPUT_EXECUTABLE}" "${DATA_DIR}"
	COMMENT "Generating ${smite_ZIP_NAME}.zip"
)

