project(lunar-lib CXX)

add_library(lunar-grammar src/grammar.cpp
                          src/concatenate.cpp
                          src/ast.cpp
)
target_link_libraries(lunar-grammar Boost::boost
                                    Boost::iostreams
                                    path)

target_include_directories(lunar-grammar PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src
)

target_compile_definitions(lunar-grammar
    PRIVATE $<${HAVE_BUILTIN_EXPECT}:HAVE_BUILTIN_EXPECT>
)

if(NOT BUILD_TESTING)
    return()
endif()

file(COPY decks/ DESTINATION decks)

add_executable(testsuite tests/testsuite.cpp
                         tests/basic-rules.cpp
                         tests/include.cpp
                         tests/ast.cpp
)

target_link_libraries(testsuite lunar-grammar catch2)
target_include_directories(testsuite PRIVATE src)
add_test(NAME testsuite COMMAND testsuite)

add_executable(concatenate-benchmark benchmarks/concatenate.cpp)
target_link_libraries(concatenate-benchmark lunar-grammar)
target_include_directories(concatenate-benchmark PRIVATE src)
