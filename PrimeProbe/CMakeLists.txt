cmake_minimum_required(VERSION 3.30)
project(PrimeProbe C)

set(CMAKE_C_STANDARD 17)

# Create a library for shared functionality
add_library(PrimeProbeLib
        shared.c
        shared.h
)

# Export the include directory for other subprojects
target_include_directories(PrimeProbeLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(PrimeProbe main.c)
