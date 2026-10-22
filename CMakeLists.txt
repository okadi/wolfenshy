cmake_minimum_required(VERSION 3.25)
project(wolfenshy C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# libraries
add_subdirectory(lib/SDL EXCLUDE_FROM_ALL)

target_compile_options(SDL3-shared PRIVATE -O2 -g0)
target_link_options(SDL3-shared PRIVATE -s)

# project
add_executable(wolfenshy
	src/main.c
)

target_include_directories(wolfenshy PRIVATE
	src
)

target_link_libraries(wolfenshy PRIVATE SDL3::SDL3)
target_compile_options(wolfenshy PRIVATE -Wall -Wextra)

# linux math lib
if (UNIX AND NOT APPLE)
	target_link_libraries(wolfenshy PRIVATE m)
endif()
