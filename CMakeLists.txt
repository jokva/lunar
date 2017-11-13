cmake_minimum_required(VERSION 3.5)
project(lunar CXX)

include(CTest)
include(CheckCXXSourceCompiles)

find_package(Boost REQUIRED iostreams)

set(CMAKE_CXX_STANDARD 14)
check_cxx_source_compiles(
    "int main(){ return __builtin_expect(l,1); }"
    HAVE_BUILTIN_EXPECT
)

add_subdirectory(external/catch2)
add_subdirectory(external/filesystem)

add_subdirectory(lib)
add_subdirectory(bin)
