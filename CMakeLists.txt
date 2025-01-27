cmake_minimum_required(VERSION 3.15)

project(
	learn-vulkan
	VERSION 1.0
	DESCRIPTION "learn-vulkan"
	LANGUAGES C
	)


# generate compile_compands for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE srcfiles src/*.c src/*.h)

add_executable(learn-vulkan ${srcfiles})

target_compile_options(learn-vulkan PRIVATE -Wall -g)

target_include_directories(learn-vulkan PRIVATE src)

find_package(OpenAL REQUIRED)

find_library(CLIB_LIB clib HINTS /usr/lib/clib)

# find_package(cglm CONFIG REQUIRED)
target_link_libraries( learn-vulkan
	PRIVATE
	vulkan
	glfw3
	X11 # for glfw3
	m # math
	cglm
	${CLIB_LIB}
	)


