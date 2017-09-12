cmake_minimum_required(VERSION 3.5)
project(lunar CXX)

enable_testing()

find_package(Boost REQUIRED unit_test_framework)

set(CMAKE_CXX_STANDARD 14)
add_subdirectory(lib)
