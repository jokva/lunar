project(lunar-bin CXX)

add_executable(umbra umbra.cpp)
target_link_libraries(umbra lunar-grammar)
