project(lunar-lib CXX)

add_executable(lunar src/main.cpp
                     src/grammar.cpp)
target_link_libraries(lunar Boost::boost)

target_include_directories(lunar PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
