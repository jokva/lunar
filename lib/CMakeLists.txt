project(lunar-lib CXX)

add_library(lunar-grammar src/grammar.cpp)
target_link_libraries(lunar-grammar Boost::boost)

target_include_directories(lunar-grammar PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_executable(umbra src/umbra.cpp)
target_link_libraries(umbra lunar-grammar)

add_executable(runspec-tests tests/runspec.cpp)
target_link_libraries(runspec-tests Boost::unit_test_framework lunar-grammar)
target_compile_definitions(runspec-tests PUBLIC -DBOOST_TEST_DYN_LINK)
add_test(NAME runspec COMMAND runspec-tests)
