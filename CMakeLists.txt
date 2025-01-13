cmake_minimum_required(VERSION 3.30)
project(Michaels_PP)

set(CMAKE_C_STANDARD 17)

# Add each subdirectory
add_subdirectory(receiver)
add_subdirectory(sender)
add_subdirectory(PrimeProbe)


# Add an executable target
add_executable(Michaels_PP main.c)

add_dependencies(Michaels_PP receiver sender PrimeProbe)