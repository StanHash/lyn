project(lyn)
cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(SOURCE_LIST
    main.cpp

    core/binary_file.h
    core/binary_file.cpp

    elf/raw_elf.h

    elf/elf_file.h
    elf/elf_file.cpp

    core/event_code.h
    core/event_code.cpp

    core/event_section.h
    core/event_section.cpp

    event_object.h
    event_object.cpp
)

add_executable(${PROJECT_NAME} ${SOURCE_LIST})
