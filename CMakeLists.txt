cmake_minimum_required(VERSION 3.5)
project(lunar CXX)

include(CTest)

find_package(Boost REQUIRED iostreams)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(external/catch2)

add_subdirectory(lib)
add_subdirectory(bin)
