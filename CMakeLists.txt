cmake_minimum_required(VERSION 3.9)
project(screwme)

set(CMAKE_CXX_STANDARD 17)

set(CXXFLAGS "${CXXFLAGS} -std=c++11 -Wall -O2 -pthread")

add_executable(screwme main.cpp)
target_link_libraries(screwme pthread)