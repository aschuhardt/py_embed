cmake_minimum_required(VERSION 3.12.4 FATAL_ERROR)

set(CMAKE_BUILD_TYPE Debug)

project(py_embed VERSION 0.1.0 LANGUAGES C)

include(FindPythonLibs)

find_package(Python3 COMPONENTS Development)

message(${Python3_INCLUDE_DIRS})

add_library(py_embed STATIC src/py_embed.c)
target_include_directories(py_embed PUBLIC include)
target_include_directories(py_embed PRIVATE ${Python3_INCLUDE_DIRS})
target_link_libraries(py_embed ${Python3_LIBRARIES})

add_executable(testapp src/test.c)
target_link_libraries(testapp py_embed)
add_custom_target(memcheck COMMAND debug/memcheck.sh DEPENDS testapp)