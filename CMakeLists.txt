cmake_minimum_required(VERSION 3.8)
project(fan_texturing)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS KEEP_RPATHS)

add_subdirectory(externals)
add_subdirectory(src)