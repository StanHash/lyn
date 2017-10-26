project(lyn)
cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(SOURCE_LIST
    main.cpp

    elf/raw_elf.h

    elf/elf.h
    elf/elf.cpp

    core/binary_file.h
    core/binary_file.cpp

    core/event_section.h
    core/event_section.cpp
)

add_executable(${PROJECT_NAME} ${SOURCE_LIST})
