cmake_minimum_required(VERSION 3.14)
project(smite_editor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(
	glfw
	GIT_REPOSITORY https://github.com/glfw/glfw.git
	GIT_TAG 3.4
	GIT_SHALLOW TRUE
)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
	glm
	GIT_REPOSITORY https://github.com/g-truc/glm.git
	GIT_TAG 1.0.1
	GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glm)

# Docking + latest stable line: use *-docking tag (see https://github.com/ocornut/imgui/wiki/Docking)
FetchContent_Declare(
	imgui
	GIT_REPOSITORY https://github.com/ocornut/imgui.git
	GIT_TAG v1.92.7-docking
	GIT_SHALLOW TRUE
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
	FetchContent_Populate(imgui)
endif()

set(IMGUI_DIR ${imgui_SOURCE_DIR})
add_executable(smite_editor
	src/main.cpp
	src/formats.cpp
	src/atlas_pack.cpp
	src/gl_extra.cpp
	src/dungeon_gl.cpp
	src/dungeon_atlas.cpp
	${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
	${IMGUI_DIR}/imgui.cpp
	${IMGUI_DIR}/imgui_draw.cpp
	${IMGUI_DIR}/imgui_tables.cpp
	${IMGUI_DIR}/imgui_widgets.cpp
	${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
	${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(smite_editor PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${IMGUI_DIR}
	${IMGUI_DIR}/backends
)
target_link_libraries(smite_editor PRIVATE glfw glm::glm)
if(WIN32)
	target_link_libraries(smite_editor PRIVATE opengl32)
	# Avoid min/max macros from windows.h breaking std::min / std::max (e.g. in dungeon_gl.cpp).
	target_compile_definitions(smite_editor PRIVATE NOMINMAX)
else()
	find_package(OpenGL REQUIRED)
	target_link_libraries(smite_editor PRIVATE OpenGL::GL)
endif()
if(MSVC)
	target_compile_definitions(smite_editor PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
target_compile_definitions(smite_editor PRIVATE IMGUI_DEFINE_MATH_OPERATORS)

# Baked path so default dungeon textures load even when CWD is not the exe directory (e.g. VS debugger).
set(SMITE_EDITOR_SAMPLE_TEX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sample_textures")
target_compile_definitions(smite_editor PRIVATE "SMITE_EDITOR_SAMPLE_TEX_DIR=\"${SMITE_EDITOR_SAMPLE_TEX_DIR}\"")

add_custom_command(
	TARGET smite_editor POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/sample_textures"
		"$<TARGET_FILE_DIR:smite_editor>/sample_textures"
	COMMENT "Copy dungeon atlas sample textures next to smite_editor"
)
