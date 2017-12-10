project(lyn)
cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")

set(SOURCE_LIST
    main.cpp

    core/util.h
    core/util.cpp

    core/binary_file.h
    core/binary_file.cpp

    elf/raw_elf.h

    elf/elf_file.h
    elf/elf_file.cpp

    ea/event_code.h
    ea/event_code.cpp

    ea/event_section.h
    ea/event_section.cpp

    core/section_data.h
    core/section_data.cpp

    core/event_object.h
    core/event_object.cpp

    core/arm_relocator.h
    core/arm_relocator.cpp
)

add_executable(${PROJECT_NAME} ${SOURCE_LIST})
