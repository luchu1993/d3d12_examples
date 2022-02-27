# Set CMake minimum version
cmake_minimum_required(VERSION 3.20.0)

# Set project name
project(D3D12Examples)

# Set CMake modules search path
set (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

#Include Common.cmake
include(Common)

add_subdirectory(Example)
