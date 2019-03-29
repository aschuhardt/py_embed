cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(py_embed VERSION 0.1.0 LANGUAGES C)

include(FindPythonLibs)

find_package(PythonLibs REQUIRED)

add_library(py_embed src/py_embed.c)
target_include_directories(py_embed PUBLIC include)
target_include_directories(py_embed PRIVATE "${PYTHON_INCLUDE_DIRS}")
target_link_libraries(py_embed "${PYTHON_LIBRARIES}")

add_executable(testapp src/test.c)
target_link_libraries(testapp py_embed)