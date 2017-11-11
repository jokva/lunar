project(lunar-bin CXX)

add_executable(umbra umbra.cpp)
target_link_libraries(umbra lunar-grammar)

add_executable(inline inline.cpp)
target_link_libraries(inline lunar-grammar)
