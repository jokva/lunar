cmake_minimum_required(VERSION 3.0)
project(path CXX)

add_library(path INTERFACE)
target_include_directories(path SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
