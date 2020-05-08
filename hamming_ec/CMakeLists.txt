cmake_minimum_required(VERSION 3.0)
project(hamming_ec)

set(CMAKE_CXX_STANDARD 11)

add_executable(
        hamming_ec

        main.cpp
        HammingEC.cpp
        HammingEC.hpp
)
