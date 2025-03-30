cmake_minimum_required(VERSION 3.30)
project(sender C)
set(CMAKE_C_STANDARD 17)

#add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g -O0)
#add_link_options(-fsanitize=address)

include_directories(${CMAKE_SOURCE_DIR}/Mastik-main/mastik)


# Add Mastik source files
set(MASTIK_SOURCES
        ${CMAKE_SOURCE_DIR}/Mastik-main/src/cb.c
        ${CMAKE_SOURCE_DIR}/Mastik-main/src/ff.c
        ${CMAKE_SOURCE_DIR}/Mastik-main/src/fr.c
        ${CMAKE_SOURCE_DIR}/Mastik-main/src/l2.c
        ${CMAKE_SOURCE_DIR}/Mastik-main/src/l3.c
        ${CMAKE_SOURCE_DIR}/Mastik-main/src/lx.c
        ${CMAKE_SOURCE_DIR}/Mastik-main/src/mm.c
        ${CMAKE_SOURCE_DIR}/Mastik-main/src/pda.c
        ${CMAKE_SOURCE_DIR}/Mastik-main/src/symbol.c
        ${CMAKE_SOURCE_DIR}/Mastik-main/src/synctrace.c
        ${CMAKE_SOURCE_DIR}/Mastik-main/src/util.c
)

add_executable(sender main.c ${LIB_SOURCES}
        correlated_set.c
        correlated_set.h
        utils.c
        utils.h
        ../PrimeProbe/shared.c
        ../PrimeProbe/shared.h)
target_link_libraries(sender pthread)
target_link_libraries(sender ${CMAKE_SOURCE_DIR}/Mastik-main/src/libmastik.a)