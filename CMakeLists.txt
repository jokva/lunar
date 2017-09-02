cmake_minimum_required(VERSION 3.5)
project(lunar CXX)

find_package(Boost REQUIRED)

set(CMAKE_CXX_STANDARD 14)
add_subdirectory(lib)
