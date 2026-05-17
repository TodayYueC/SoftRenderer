cmake_minimum_required(VERSION 4.2)
project(SoftRenderer)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenMP REQUIRED)

add_executable(SoftRenderer main.cpp
        tgaimage.h
        tgaimage.cpp
        geometry.h
        model.h
        model.cpp)

target_link_libraries(SoftRenderer PRIVATE OpenMP::OpenMP_CXX)
