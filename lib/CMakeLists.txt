project(lunar-lib CXX)

add_library(lunar-grammar src/grammar.cpp)
target_link_libraries(lunar-grammar Boost::boost)

target_include_directories(lunar-grammar PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_executable(basic-rules tests/basic-rules.cpp)
target_link_libraries(basic-rules Boost::unit_test_framework lunar-grammar)
target_compile_definitions(basic-rules PUBLIC -DBOOST_TEST_DYN_LINK)
add_test(NAME basic-rules COMMAND basic-rules)
