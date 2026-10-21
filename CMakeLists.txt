cmake_minimum_required(VERSION 3.25)
project(swr C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# libraries
add_subdirectory(lib/SDL EXCLUDE_FROM_ALL)

target_compile_options(SDL3-shared PRIVATE -O2 -g0)
target_link_options(SDL3-shared PRIVATE -s)

# project
add_executable(swr
	src/main.c
)

target_include_directories(swr PRIVATE
	src
)

target_compile_options(swr PRIVATE -Wall -Wextra)
target_link_libraries(swr PRIVATE SDL3::SDL3)

if (UNIX AND NOT APPLE)
	target_link_libraries(swr PRIVATE m)
endif()
