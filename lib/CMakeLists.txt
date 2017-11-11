project(lunar-lib CXX)

add_library(lunar-grammar src/grammar.cpp)
target_link_libraries(lunar-grammar Boost::boost)

target_include_directories(lunar-grammar PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(NOT BUILD_TESTING)
    return()
endif()

add_executable(testsuite tests/testsuite.cpp
                         tests/basic-rules.cpp
)
target_link_libraries(testsuite lunar-grammar catch2)
add_test(NAME testsuite COMMAND testsuite)
