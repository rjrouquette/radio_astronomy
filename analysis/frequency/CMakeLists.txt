cmake_minimum_required(VERSION 3.0)
project(frequency_response)

set(CMAKE_CXX_STANDARD 11)

add_executable(
        frequency_response

        main.cpp
        pipe.cpp
        pipe.h
        module.cpp
        module.h
        mixer.cpp
        mixer.h
        sine.cpp
        sine.h
        butter.cpp
        butter.h
        fir.cpp
        fir.h
)

target_link_libraries(frequency_response -lfftw3)

