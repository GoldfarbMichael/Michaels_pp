cmake_minimum_required(VERSION 3.30)
project(receiver C)
set(CMAKE_C_STANDARD 17)


include_directories(/home/michael/CLionProjects/CovertChannel/Mastik-main/mastik)

# Add Mastik source files
set(MASTIK_SOURCES
        /home/michael/CLionProjects/CovertChannel/Mastik-main/src/cb.c
        /home/michael/CLionProjects/CovertChannel/Mastik-main/src/ff.c
        /home/michael/CLionProjects/CovertChannel/Mastik-main/src/fr.c
        /home/michael/CLionProjects/CovertChannel/Mastik-main/src/l2.c
        /home/michael/CLionProjects/CovertChannel/Mastik-main/src/l3.c
        /home/michael/CLionProjects/CovertChannel/Mastik-main/src/lx.c
        /home/michael/CLionProjects/CovertChannel/Mastik-main/src/mm.c
        /home/michael/CLionProjects/CovertChannel/Mastik-main/src/pda.c
        /home/michael/CLionProjects/CovertChannel/Mastik-main/src/symbol.c
        /home/michael/CLionProjects/CovertChannel/Mastik-main/src/synctrace.c
        /home/michael/CLionProjects/CovertChannel/Mastik-main/src/util.c
)

add_executable(receiver main.c ${LIB_SOURCES}
        training.c
        training.h
        prepare_receiver.c
        prepare_receiver.h)

target_link_libraries(receiver PRIVATE /home/michael/CLionProjects/CovertChannel/Mastik-main/src/libmastik.a)